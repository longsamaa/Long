#version 430

// Physically-based shading: Cook-Torrance specular (GGX distribution, Smith
// geometry, Schlick fresnel) + Lambert diffuse, in HDR (tonemap happens later
// in the pipeline). SAME light/shadow uniform interface as default.frag, so
// CommandQueue::BindLights/BindShadow feed this shader with no C++ changes.

in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragNormal;
in vec3 fragPosition;   // world-space

uniform sampler2D texture0;   // albedo map (raylib sets this; white 1x1 default)

// Extra PBR maps. GLRenderer::ApplyMaterial binds a 1x1 white default when the
// material doesn't provide one, so sampling is always safe and the u_* factors
// below stay in charge.
uniform sampler2D u_texMetalRough; // glTF packing: G = roughness, B = metallic
uniform sampler2D u_texEmissive;   // sRGB
uniform sampler2D u_texOcclusion;  // R = ambient occlusion

// Surface params (BaseMaterial u_* uniforms).
uniform vec4  u_baseColor;          // albedo tint
uniform vec3  u_emissive;           // emissive color (0..1)
uniform float u_emissiveIntensity;  // HDR multiplier for emissive
uniform float u_metallic;           // 0 = dielectric, 1 = metal
uniform float u_roughness;          // 0 = mirror, 1 = matte
uniform float u_ao;                 // ambient occlusion factor

// ---- Lights: identical layout to default.frag ----
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

// Hemisphere ambient from the gradient skybox (bound by BindLights): the sky
// color lights up-facing surfaces, the ground color down-facing ones.
uniform vec3  u_ambientSky;
uniform vec3  u_ambientGround;
uniform float u_ambientIntensity;

// ---- Shadows: identical layout to default.frag ----
#define MAX_SHADOWS 4
uniform int       u_shadowCount;
uniform int       u_receiveShadow;
uniform float     u_shadowOpacity;
uniform mat4      u_lightViewProj[MAX_SHADOWS];
uniform sampler2D u_shadowMaps[MAX_SHADOWS];

// Point-light omnidirectional shadows: a depth cube storing linear distance/range.
#define MAX_CUBE_SHADOWS 2
uniform int         u_cubeShadowCount;
uniform samplerCube u_pointShadowMaps[MAX_CUBE_SHADOWS];
uniform vec3        u_pointLightPos[MAX_CUBE_SHADOWS];
uniform float       u_pointLightRange[MAX_CUBE_SHADOWS];

// 0 = shadowed, 1 = lit. Compares this fragment's distance-to-light against the
// nearest occluder distance stored in the cube (sampled by that same direction).
float PointShadowFactor(int idx, vec3 worldPos)
{
    if (idx < 0 || idx >= u_cubeShadowCount || u_receiveShadow == 0) return 1.0;
    vec3 toFrag = worldPos - u_pointLightPos[idx];
    float current = length(toFrag) / max(u_pointLightRange[idx], 1e-4);
    float closest = texture(u_pointShadowMaps[idx], toFrag).r;
    // Small constant bias on the normalized [0,1] distance. Too large (0.02 =
    // 0.4 world units at range 20) inflates the occluder and the shadow balloons.
    float lit = (current - 0.003 > closest) ? 0.0 : 1.0;
    return mix(1.0, lit, clamp(u_shadowOpacity, 0.0, 1.0));
}

out vec4 finalColor;

const float PI = 3.14159265359;

// Same as default.frag: 0 = shadowed, 1 = lit. 3x3 bilinear PCF + slope bias.
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

// GGX/Trowbridge-Reitz normal distribution: how many microfacets face H.
float DistributionGGX(float ndh, float roughness)
{
    float a  = roughness * roughness; // Disney remap: perceptually linear
    float a2 = a * a;
    float d  = ndh * ndh * (a2 - 1.0) + 1.0;
    return a2 / max(PI * d * d, 1e-6);
}

// Smith height-correlated visibility (Schlick-GGX approximation).
float GeometrySmith(float ndv, float ndl, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0; // direct-lighting remap
    float gv = ndv / (ndv * (1.0 - k) + k);
    float gl = ndl / (ndl * (1.0 - k) + k);
    return gv * gl;
}

// Schlick fresnel: reflectance grows toward 1 at grazing angles.
vec3 FresnelSchlick(float hdv, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - hdv, 0.0, 1.0), 5.0);
}

// Fresnel for AMBIENT: rough surfaces can't reach full grazing reflectance,
// so the "ceiling" is pulled down by roughness (standard IBL trick).
vec3 FresnelSchlickRoughness(float ndv, vec3 F0, float roughness)
{
    vec3 Fmax = max(vec3(1.0 - roughness), F0);
    return F0 + (Fmax - F0) * pow(clamp(1.0 - ndv, 0.0, 1.0), 5.0);
}

void main()
{
    // NOTE: no sRGB decode on purpose. The whole engine feeds sRGB-encoded
    // colors (u_baseColor, light colors) straight into lighting, and the look
    // is calibrated around that. Decoding only textures makes textured models
    // darker than everything else. If proper linear workflow is ever wanted,
    // decode ALL color inputs at once, not just this one.
    vec4 baseTex = texture(texture0, fragTexCoord) * u_baseColor * fragColor;
    vec3 albedo = baseTex.rgb;
    // Map values MULTIPLY the material factors (glTF semantics): the white
    // default leaves the u_* factors in charge.
    vec3 mrTex = texture(u_texMetalRough, fragTexCoord).rgb;
    float metallic  = clamp(u_metallic * mrTex.b, 0.0, 1.0);
    float roughness = clamp(u_roughness * mrTex.g, 0.04, 1.0); // 0 breaks the NDF
    float ao = clamp(u_ao * texture(u_texOcclusion, fragTexCoord).r, 0.0, 1.0);
    vec3 N = normalize(fragNormal);
    vec3 V = normalize(u_viewPos - fragPosition);
    float ndv = max(dot(N, V), 1e-4);

    // Base reflectance: dielectrics ~4%, metals reflect with their albedo.
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
            vec3 toLight = lt.position - fragPosition;
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

        // Punctual-light convention: fold PI into the light so intensity 1 gives
        // peak diffuse = albedo (the BRDF's albedo/PI would otherwise make PBR
        // lights 3x dimmer than the engine's Blinn-Phong shaders at the same
        // intensity, letting ambient wash everything flat).
        float shadow = ShadowFactor(lt.shadowIndex, fragPosition, N, L)
                     * PointShadowFactor(lt.cubeShadowIndex, fragPosition);
        vec3 radiance = lt.color.rgb * (lt.intensity * atten * PI) * shadow;

        // Cook-Torrance specular term.
        vec3  H   = normalize(V + L);
        float ndh = max(dot(N, H), 0.0);
        float hdv = max(dot(H, V), 0.0);
        float NDF = DistributionGGX(ndh, roughness);
        float G   = GeometrySmith(ndv, ndl, roughness);
        vec3  F   = FresnelSchlick(hdv, F0);
        vec3 specular = (NDF * G * F) / max(4.0 * ndv * ndl, 1e-4);

        // Energy conservation: what reflects specularly can't diffuse; metals
        // have no diffuse at all.
        vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);
        Lo += (kD * albedo / PI + specular) * radiance * ndl;
    }

    // --- Hemisphere ambient (poor man's IBL, follows the gradient skybox) ---
    // Diffuse irradiance: blend ground->sky by how much the normal faces up.
    vec3 irradiance = mix(u_ambientGround, u_ambientSky, N.y * 0.5 + 0.5);
    vec3 kD_amb = (vec3(1.0) - FresnelSchlickRoughness(ndv, F0, roughness))
                * (1.0 - metallic);
    vec3 diffuseAmb = kD_amb * irradiance * albedo;

    // Specular ambient: sky color along the reflection vector, blurred toward
    // plain irradiance as roughness grows (a rough mirror reflects "everything").
    vec3 R = reflect(-V, N);
    vec3 reflectedSky = mix(u_ambientGround, u_ambientSky, R.y * 0.5 + 0.5);
    vec3 specAmb = mix(reflectedSky, irradiance, roughness)
                 * FresnelSchlickRoughness(ndv, F0, roughness);

    vec3 ambient = (diffuseAmb + specAmb) * ao * u_ambientIntensity;

    // Emissive is HDR: intensity > 1 feeds the bloom pipeline.
    vec3 emissive = u_emissive * u_emissiveIntensity
                  * texture(u_texEmissive, fragTexCoord).rgb;

    finalColor = vec4(ambient + Lo + emissive, baseTex.a);
}
