#version 430

// Default lit surface -- PBR (same shading as pbr.frag): Cook-Torrance GGX
// specular + Lambert diffuse + hemisphere ambient from the gradient skybox.
// HDR output; ACES tonemap happens later in the pipeline.

in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragNormal;
in vec3 fragPosition;   // world-space

uniform sampler2D texture0;   // albedo map (raylib sets this; white 1x1 default)

// Surface params (BaseMaterial u_* uniforms).
uniform vec4  u_baseColor;          // albedo tint
uniform vec3  u_emissive;           // emissive color (0..1)
uniform float u_emissiveIntensity;  // HDR multiplier for emissive
uniform float u_metallic;           // 0 = dielectric, 1 = metal
uniform float u_roughness;          // 0 = mirror, 1 = matte
uniform float u_ao;                 // ambient occlusion factor

// ---- Lights (uploaded by CommandQueue::BindLights) ----
#define MAX_LIGHTS 8
#define LIGHT_DIRECTIONAL 0
#define LIGHT_POINT 1
#define LIGHT_SPOT 2

struct Light {
    vec3  position;
    vec3  direction;
    vec4  color;
    float intensity;
    int   type;
    float innerCos;
    float outerCos;
    float range;
    int   shadowIndex;
    int   cubeShadowIndex;
};

uniform int   u_lightCount;
uniform Light u_lights[MAX_LIGHTS];
uniform vec3  u_viewPos;

// Hemisphere ambient from the gradient skybox (bound by BindLights).
uniform vec3  u_ambientSky;
uniform vec3  u_ambientGround;
uniform float u_ambientIntensity;

// ---- Shadows (uploaded by CommandQueue::BindShadow) ----
#define MAX_SHADOWS 4
uniform int       u_shadowCount;
uniform int       u_receiveShadow;
uniform float     u_shadowOpacity;
uniform mat4      u_lightViewProj[MAX_SHADOWS];
uniform sampler2D u_shadowMaps[MAX_SHADOWS];

// Point-light omnidirectional shadows (see pbr.frag).
#define MAX_CUBE_SHADOWS 2
uniform int         u_cubeShadowCount;
uniform samplerCube u_pointShadowMaps[MAX_CUBE_SHADOWS];
uniform vec3        u_pointLightPos[MAX_CUBE_SHADOWS];
uniform float       u_pointLightRange[MAX_CUBE_SHADOWS];

float PointShadowFactor(int idx, vec3 worldPos)
{
    if (idx < 0 || idx >= u_cubeShadowCount || u_receiveShadow == 0) return 1.0;
    vec3 toFrag = worldPos - u_pointLightPos[idx];
    float current = length(toFrag) / max(u_pointLightRange[idx], 1e-4);
    float closest = texture(u_pointShadowMaps[idx], toFrag).r;
    float lit = (current - 0.003 > closest) ? 0.0 : 1.0;
    return mix(1.0, lit, clamp(u_shadowOpacity, 0.0, 1.0));
}

out vec4 finalColor;

const float PI = 3.14159265359;

// 0 = fully shadowed, 1 = fully lit. 3x3 bilinear PCF + slope-scaled bias.
// Keep in sync with pbr.frag / wireframe.frag.
float ShadowFactor(int idx, vec3 worldPos, vec3 N, vec3 L)
{
    if (idx < 0 || idx >= u_shadowCount || u_receiveShadow == 0) return 1.0;
    vec4 lp = u_lightViewProj[idx] * vec4(worldPos, 1.0);
    vec3 proj = lp.xyz / lp.w;
    proj = proj * 0.5 + 0.5;
    if (proj.z > 1.0 ||
        proj.x < 0.0 || proj.x > 1.0 ||
        proj.y < 0.0 || proj.y > 1.0) {
        return 1.0;
    }
    float bias = max(0.0025 * (1.0 - dot(N, L)), 0.0005);
    float compare = proj.z - bias;
    vec2 texel = 1.0 / vec2(textureSize(u_shadowMaps[idx], 0));
    float shadow = 0.0;
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            vec2 uv = proj.xy + vec2(x, y) * texel;
            vec2 pos = uv / texel - 0.5;
            vec2 f = fract(pos);
            vec2 base = (floor(pos) + 0.5) * texel;
            float bl = (compare > texture(u_shadowMaps[idx], base).r) ? 0.0 : 1.0;
            float br = (compare > texture(u_shadowMaps[idx], base + vec2(texel.x, 0.0)).r) ? 0.0 : 1.0;
            float tl = (compare > texture(u_shadowMaps[idx], base + vec2(0.0, texel.y)).r) ? 0.0 : 1.0;
            float tr = (compare > texture(u_shadowMaps[idx], base + texel).r) ? 0.0 : 1.0;
            shadow += mix(mix(bl, br, f.x), mix(tl, tr, f.x), f.y);
        }
    }
    return mix(1.0, shadow / 9.0, clamp(u_shadowOpacity, 0.0, 1.0));
}

// ---- Cook-Torrance BRDF terms (keep in sync with pbr.frag) ----
float DistributionGGX(float ndh, float roughness)
{
    float a  = roughness * roughness;
    float a2 = a * a;
    float d  = ndh * ndh * (a2 - 1.0) + 1.0;
    return a2 / max(PI * d * d, 1e-6);
}
float GeometrySmith(float ndv, float ndl, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    float gv = ndv / (ndv * (1.0 - k) + k);
    float gl = ndl / (ndl * (1.0 - k) + k);
    return gv * gl;
}
vec3 FresnelSchlick(float hdv, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - hdv, 0.0, 1.0), 5.0);
}
vec3 FresnelSchlickRoughness(float ndv, vec3 F0, float roughness)
{
    vec3 Fmax = max(vec3(1.0 - roughness), F0);
    return F0 + (Fmax - F0) * pow(clamp(1.0 - ndv, 0.0, 1.0), 5.0);
}

// Direct + ambient PBR lighting for one surface point.
vec3 ComputePBR(vec3 albedo, float alpha_unused, vec3 N, vec3 worldPos)
{
    float metallic  = clamp(u_metallic, 0.0, 1.0);
    float roughness = clamp(u_roughness, 0.04, 1.0);
    vec3 V = normalize(u_viewPos - worldPos);
    float ndv = max(dot(N, V), 1e-4);
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    vec3 Lo = vec3(0.0);
    for (int i = 0; i < u_lightCount; ++i) {
        Light lt = u_lights[i];
        vec3 L;
        float atten = 1.0;
        if (lt.type == LIGHT_DIRECTIONAL) {
            L = normalize(-lt.direction);
        }
        else {
            vec3 toLight = lt.position - worldPos;
            float dist = length(toLight);
            L = toLight / max(dist, 1e-4);
            float falloff = clamp(1.0 - dist / max(lt.range, 1e-3), 0.0, 1.0);
            atten = falloff * falloff;
            if (lt.type == LIGHT_SPOT) {
                float cd = dot(-L, normalize(lt.direction));
                atten *= smoothstep(lt.outerCos, lt.innerCos, cd);
            }
        }
        float ndl = max(dot(N, L), 0.0);
        if (ndl <= 0.0 || atten <= 0.0) continue;

        // Punctual-light convention: PI folded into the light (see pbr.frag).
        vec3 radiance = lt.color.rgb * (lt.intensity * atten * PI)
                      * ShadowFactor(lt.shadowIndex, worldPos, N, L)
                      * PointShadowFactor(lt.cubeShadowIndex, worldPos);

        vec3  H   = normalize(V + L);
        float ndh = max(dot(N, H), 0.0);
        float hdv = max(dot(H, V), 0.0);
        float NDF = DistributionGGX(ndh, roughness);
        float G   = GeometrySmith(ndv, ndl, roughness);
        vec3  F   = FresnelSchlick(hdv, F0);
        vec3 specular = (NDF * G * F) / max(4.0 * ndv * ndl, 1e-4);
        vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);
        Lo += (kD * albedo / PI + specular) * radiance * ndl;
    }

    // Hemisphere ambient (gradient skybox).
    vec3 irradiance = mix(u_ambientGround, u_ambientSky, N.y * 0.5 + 0.5);
    vec3 Famb = FresnelSchlickRoughness(ndv, F0, roughness);
    vec3 kD_amb = (vec3(1.0) - Famb) * (1.0 - metallic);
    vec3 diffuseAmb = kD_amb * irradiance * albedo;
    vec3 R = reflect(-V, N);
    vec3 reflectedSky = mix(u_ambientGround, u_ambientSky, R.y * 0.5 + 0.5);
    vec3 specAmb = mix(reflectedSky, irradiance, roughness) * Famb;
    vec3 ambient = (diffuseAmb + specAmb)
                 * clamp(u_ao, 0.0, 1.0) * u_ambientIntensity;

    return ambient + Lo + u_emissive * u_emissiveIntensity;
}

void main()
{
    vec4 base = texture(texture0, fragTexCoord) * u_baseColor * fragColor;
    vec3 N = normalize(fragNormal);
    finalColor = vec4(ComputePBR(base.rgb, base.a, N, fragPosition), base.a);
}
