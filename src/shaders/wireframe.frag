#version 430

// Lit cube (same lighting as default.frag) PLUS a box-edge wireframe on top.
// Each cube face has UV in [0,1], so fragments near a UV border lie on a face
// edge -- painting those gives the 12 box edges (no face diagonals).

in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragNormal;
in vec3 fragPosition;   // world-space

uniform sampler2D texture0;   // diffuse map (raylib sets this)

// Our uniforms (raylib doesn't touch the u_ prefix).
uniform vec4  u_faceColor;  // cube tint (like default's u_baseColor)
uniform vec4  u_lineColor;  // edge color
uniform float u_thickness;  // edge width in UV space (0..0.5), e.g. 0.03

// Scene lights -- SAME interface as default.frag, so CommandQueue::BindLights
// uploads them here too (matching uniform names). MAX_LIGHTS must match
// SceneLights::kMaxLights on the C++ side.
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
};

uniform int   u_lightCount;
uniform Light u_lights[MAX_LIGHTS];

// Specular params -- same as default.frag (u_viewPos uploaded per-shader).
uniform vec3  u_viewPos;
uniform float u_roughness;
uniform float u_metallic;

// Shadow mapping -- same interface as default.frag (bound by BindShadow).
uniform int       u_shadowEnabled;
uniform int       u_receiveShadow;
uniform float     u_shadowOpacity;
uniform mat4      u_lightViewProj;
uniform sampler2D u_shadowMap;

out vec4 finalColor;

// Same as default.frag: 0 = shadowed, 1 = lit. 3x3 PCF + slope-scaled bias.
float ShadowFactor(vec3 worldPos, vec3 N, vec3 L)
{
    if (u_shadowEnabled == 0 || u_receiveShadow == 0) return 1.0;
    vec4 lp = u_lightViewProj * vec4(worldPos, 1.0);
    vec3 proj = lp.xyz / lp.w;
    proj = proj * 0.5 + 0.5;
    if (proj.z > 1.0 ||
        proj.x < 0.0 || proj.x > 1.0 ||
        proj.y < 0.0 || proj.y > 1.0) {
        return 1.0;
    }
    float bias = max(0.0025 * (1.0 - dot(N, L)), 0.0005);
    float compare = proj.z - bias;
    vec2 texel = 1.0 / vec2(textureSize(u_shadowMap, 0));
    // 3x3 bilinear PCF -- keep in sync with default.frag.
    float shadow = 0.0;
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            vec2 uv = proj.xy + vec2(x, y) * texel;
            vec2 pos = uv / texel - 0.5;
            vec2 f = fract(pos);
            vec2 base = (floor(pos) + 0.5) * texel;
            float bl = (compare > texture(u_shadowMap, base).r) ? 0.0 : 1.0;
            float br = (compare > texture(u_shadowMap, base + vec2(texel.x, 0.0)).r) ? 0.0 : 1.0;
            float tl = (compare > texture(u_shadowMap, base + vec2(0.0, texel.y)).r) ? 0.0 : 1.0;
            float tr = (compare > texture(u_shadowMap, base + texel).r) ? 0.0 : 1.0;
            shadow += mix(mix(bl, br, f.x), mix(tl, tr, f.x), f.y);
        }
    }
    // Per-material opacity -- keep in sync with default.frag.
    return mix(1.0, shadow / 9.0, clamp(u_shadowOpacity, 0.0, 1.0));
}

// Identical to default.frag's ComputeLighting (GLSL has no #include, so the
// shared lit shaders duplicate it; keep the two in sync).
void ComputeLighting(vec3 N, vec3 worldPos, out vec3 outDiffuse, out vec3 outSpecular)
{
    vec3 V = normalize(u_viewPos - worldPos);
    float shininess = mix(128.0, 4.0, clamp(u_roughness, 0.0, 1.0));
    float specStrength = mix(0.5, 1.0, clamp(u_metallic, 0.0, 1.0));

    outDiffuse  = vec3(0.15); // constant ambient
    outSpecular = vec3(0.0);

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
            atten = 1.0 / max(dist * dist, 1e-4);
        }
        float ndl = max(dot(N, L), 0.0);
        vec3  radiance = lt.color.rgb * (lt.intensity * atten);
        float shadow = (lt.type == LIGHT_DIRECTIONAL)
                       ? ShadowFactor(worldPos, N, L) : 1.0;
        radiance *= shadow;
        outDiffuse += radiance * ndl;
        if (ndl > 0.0) {
            vec3  H   = normalize(L + V);
            float ndh = max(dot(N, H), 0.0);
            outSpecular += radiance * (pow(ndh, shininess) * specStrength);
        }
    }
}

void main()
{
    // --- Lit base color (same lighting as default.frag) ---
    vec4 base = texture(texture0, fragTexCoord) * u_faceColor * fragColor;
    vec3 N = normalize(fragNormal);
    vec3 diffuse, specular;
    ComputeLighting(N, fragPosition, diffuse, specular);
    vec3 specTint = mix(vec3(1.0), base.rgb, clamp(u_metallic, 0.0, 1.0));
    vec3 litFace = base.rgb * diffuse + specular * specTint;

    // --- Edge mask from UV distance to the nearest face border ---
    vec2 d = min(fragTexCoord, 1.0 - fragTexCoord);
    float edge = min(d.x, d.y);
    float aa = fwidth(edge);
    float lineMask = 1.0 - smoothstep(u_thickness, u_thickness + aa, edge);

    // Edge color overrides the lit face where the mask is on.
    vec3 rgb = mix(litFace, u_lineColor.rgb, lineMask);
    finalColor = vec4(rgb, base.a);
}
