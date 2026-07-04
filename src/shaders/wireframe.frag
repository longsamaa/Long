#version 430

// PBR-lit cube (same shading as pbr.frag / default.frag) PLUS a box-edge
// wireframe on top. Each cube face has UV in [0,1], so fragments near a UV
// border lie on a face edge -- painting those gives the 12 box edges.

in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragNormal;
in vec3 fragPosition;   // world-space

uniform sampler2D texture0;   // diffuse map (raylib sets this)

// Our uniforms (raylib doesn't touch the u_ prefix).
uniform vec4  u_faceColor;  // cube tint (albedo, like default's u_baseColor)
uniform vec4  u_lineColor;  // edge color
uniform float u_thickness;  // edge width in UV space (0..0.5), e.g. 0.03

// Surface params (BaseMaterial).
uniform float u_metallic;
uniform float u_roughness;
uniform float u_ao;

// ---- Lights: identical layout to pbr.frag/default.frag ----
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
};

uniform int   u_lightCount;
uniform Light u_lights[MAX_LIGHTS];
uniform vec3  u_viewPos;

// Hemisphere ambient (gradient skybox).
uniform vec3  u_ambientSky;
uniform vec3  u_ambientGround;
uniform float u_ambientIntensity;

// ---- Shadows: identical layout to pbr.frag/default.frag ----
#define MAX_SHADOWS 4
uniform int       u_shadowCount;
uniform int       u_receiveShadow;
uniform float     u_shadowOpacity;
uniform mat4      u_lightViewProj[MAX_SHADOWS];
uniform sampler2D u_shadowMaps[MAX_SHADOWS];

out vec4 finalColor;

const float PI = 3.14159265359;

// Keep in sync with default.frag.
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

// ---- Cook-Torrance BRDF terms (keep in sync with default.frag) ----
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

// Direct + ambient PBR lighting -- keep in sync with default.frag.
vec3 ComputePBR(vec3 albedo, vec3 N, vec3 worldPos)
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
                      * ShadowFactor(lt.shadowIndex, worldPos, N, L);

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

    return ambient + Lo;
}

void main()
{
    // --- PBR-lit face ---
    vec4 base = texture(texture0, fragTexCoord) * u_faceColor * fragColor;
    vec3 N = normalize(fragNormal);
    vec3 litFace = ComputePBR(base.rgb, N, fragPosition);

    // --- Edge mask from UV distance to the nearest face border ---
    vec2 d = min(fragTexCoord, 1.0 - fragTexCoord);
    float edge = min(d.x, d.y);
    float aa = fwidth(edge);
    float lineMask = 1.0 - smoothstep(u_thickness, u_thickness + aa, edge);

    // Edge color overrides the lit face where the mask is on.
    vec3 rgb = mix(litFace, u_lineColor.rgb, lineMask);
    finalColor = vec4(rgb, base.a);
}
