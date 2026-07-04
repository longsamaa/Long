#version 430

// From the vertex shader.
in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragNormal;
in vec3 fragPosition;   // world-space

uniform sampler2D texture0;   // diffuse/albedo map (raylib sets this)

// Our OWN uniform -- raylib does not touch it, so DefaultMaterial controls it
// fully via SetUniform/ApplyUniforms.
uniform vec4 u_baseColor;

// Scene lights as a struct array (mirrors LightParameter on the C++ side).
// Uploaded ONCE per shader program by CommandQueue::Execute (BindLights).
// MAX_LIGHTS must match SceneLights::kMaxLights.
#define MAX_LIGHTS 8
#define LIGHT_DIRECTIONAL 0
#define LIGHT_POINT 1
#define LIGHT_SPOT 2

struct Light {
    vec3  position;   // world position (point/spot)
    vec3  direction;  // world direction, normalized (dir/spot)
    vec4  color;      // rgb 0..1 (+ a)
    float intensity;
    int   type;       // LightType on the C++ side
    float innerCos;   // spot: cos(inner half-angle), full brightness inside
    float outerCos;   // spot: cos(outer half-angle), zero outside
    float range;      // point/spot reach in world units
    int   shadowIndex; // entry in u_shadowMaps/u_lightViewProj, or -1 = no shadow
};

uniform int   u_lightCount;            // defaults to 0 when nothing bound
uniform Light u_lights[MAX_LIGHTS];

// Surface + view params for specular. u_viewPos is uploaded per-shader by
// CommandQueue::Execute; the rest come from BaseMaterial (default when absent).
uniform vec3  u_viewPos;               // world-space camera position
uniform float u_roughness;             // 0 = mirror-sharp, 1 = matte
uniform float u_metallic;              // 0 = dielectric, 1 = metal

// Shadow mapping (bound by CommandQueue::BindShadow). Up to MAX_SHADOWS casting
// lights (directional or spot); each light's shadowIndex picks its map below.
// u_receiveShadow / u_shadowOpacity are per-material (BaseMaterial).
#define MAX_SHADOWS 4
uniform int       u_shadowCount;
uniform int       u_receiveShadow;     // default 1 (BaseMaterial)
uniform float     u_shadowOpacity;     // 0 = shadows invisible, 1 = full (BaseMaterial)
uniform mat4      u_lightViewProj[MAX_SHADOWS]; // world -> light clip space
uniform sampler2D u_shadowMaps[MAX_SHADOWS];    // depth-from-light (slots 10+)

out vec4 finalColor;

// 0 = fully shadowed, 1 = fully lit, for shadow map `idx`. Projects the world
// position into that light's clip space and compares depths with a slope-scaled
// bias + 3x3 bilinear PCF. N and L are used only for the bias.
float ShadowFactor(int idx, vec3 worldPos, vec3 N, vec3 L)
{
    if (idx < 0 || idx >= u_shadowCount || u_receiveShadow == 0) return 1.0;

    vec4 lp = u_lightViewProj[idx] * vec4(worldPos, 1.0);
    vec3 proj = lp.xyz / lp.w;          // spot is perspective: the divide matters
    proj = proj * 0.5 + 0.5;            // NDC [-1,1] -> texture [0,1]

    // Outside the light frustum (or behind far plane): treat as lit.
    if (proj.z > 1.0 ||
        proj.x < 0.0 || proj.x > 1.0 ||
        proj.y < 0.0 || proj.y > 1.0) {
        return 1.0;
    }

    // Bias grows on grazing surfaces to fight shadow acne.
    float bias = max(0.0025 * (1.0 - dot(N, L)), 0.0005);
    float compare = proj.z - bias;
    vec2 texel = 1.0 / vec2(textureSize(u_shadowMaps[idx], 0));

    // 3x3 grid of BILINEAR PCF taps. Averaging raw 0/1 comparisons stair-steps
    // at texel edges; instead each tap compares the 4 surrounding texels and
    // bilinearly blends the RESULTS -- smooth gradient inside every texel.
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
    // Per-material opacity: fade the shadow toward "fully lit".
    return mix(1.0, shadow / 9.0, clamp(u_shadowOpacity, 0.0, 1.0));
}

// Scene lighting shared by every lit shader (default, wireframe, ...). Same
// uniform interface (u_lightCount / u_lights) so CommandQueue::BindLights feeds
// them all identically. Diffuse (Lambert) + Blinn-Phong specular per light.
//   N        : world-space normal (normalized)
//   worldPos : world-space fragment position
//   outDiffuse/outSpecular are kept separate so the caller tints only the
//   diffuse by albedo -- specular highlights stay (mostly) white on dielectrics.
void ComputeLighting(vec3 N, vec3 worldPos, out vec3 outDiffuse, out vec3 outSpecular)
{
    vec3 V = normalize(u_viewPos - worldPos);
    // Roughness -> Blinn-Phong exponent. Smooth = tight/bright highlight.
    float shininess = mix(128.0, 4.0, clamp(u_roughness, 0.0, 1.0));
    // Metals push the highlight tint toward the albedo (done in main); here just
    // scale specular strength: metals shinier, matte dielectrics softer.
    float specStrength = mix(0.5, 1.0, clamp(u_metallic, 0.0, 1.0));

    outDiffuse  = vec3(0.15); // constant ambient so unlit faces still read
    outSpecular = vec3(0.0);

    for (int i = 0; i < u_lightCount; ++i) {
        Light lt = u_lights[i];
        vec3 L;
        float atten = 1.0;
        if (lt.type == LIGHT_DIRECTIONAL) {
            L = normalize(-lt.direction);  // direction points FROM the light
        }
        else {
            // Point/spot: direction to the light + range-based falloff.
            // NOT pure inverse-square (1/d^2): that is ~0.01 at 10 units, so a
            // spot at intensity 1 was invisible. Smooth quadratic window instead:
            // full near the light, fading to exactly 0 at lt.range.
            vec3 toLight = lt.position - worldPos;
            float dist = length(toLight);
            L = toLight / max(dist, 1e-4);
            float falloff = clamp(1.0 - dist / max(lt.range, 1e-3), 0.0, 1.0);
            atten = falloff * falloff;

            if (lt.type == LIGHT_SPOT) {
                // Angle between the spot axis and this fragment: -L points from
                // the light to the surface. smoothstep fades full->zero between
                // the inner and outer cone (cosines flip the edge order).
                float cd = dot(-L, normalize(lt.direction));
                atten *= smoothstep(lt.outerCos, lt.innerCos, cd);
            }
        }
        float ndl = max(dot(N, L), 0.0);
        vec3  radiance = lt.color.rgb * (lt.intensity * atten);

        // Any light with a shadow map slot darkens (directional or spot);
        // shadowIndex == -1 means this light casts no shadow.
        radiance *= ShadowFactor(lt.shadowIndex, worldPos, N, L);

        outDiffuse += radiance * ndl;

        // Blinn-Phong: half-vector between light and view. Only where the light
        // actually hits the surface (ndl > 0), so back faces get no highlight.
        if (ndl > 0.0) {
            vec3  H   = normalize(L + V);
            float ndh = max(dot(N, H), 0.0);
            outSpecular += radiance * (pow(ndh, shininess) * specStrength);
        }
    }
}

void main()
{
    // Base color = texture * our color * vertex color.
    vec4 base = texture(texture0, fragTexCoord) * u_baseColor * fragColor;
    vec3 N = normalize(fragNormal);

    vec3 diffuse, specular;
    ComputeLighting(N, fragPosition, diffuse, specular);

    // Diffuse is tinted by the surface albedo; specular sits on top. For metals
    // the highlight takes on the albedo, for dielectrics it stays white.
    vec3 specTint = mix(vec3(1.0), base.rgb, clamp(u_metallic, 0.0, 1.0));
    vec3 color = base.rgb * diffuse + specular * specTint;

    // May exceed 1.0 -- fine, the pipeline is HDR (bloom/tonemap handle it).
    finalColor = vec4(color, base.a);
}
