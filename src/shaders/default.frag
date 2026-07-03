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

// Scene light array. Uploaded ONCE per shader program by
// CommandQueue::Execute (BindLights) -- not per material, not per draw.
// Must match SceneLights::kMaxLights on the C++ side.
#define MAX_LIGHTS 8
#define LIGHT_DIRECTIONAL 0
#define LIGHT_POINT 1
#define LIGHT_SPOT 2
uniform int   u_lightCount;                 // defaults to 0 when nothing bound
uniform vec3  u_lightPos[MAX_LIGHTS];       // world position (point/spot)
uniform vec3  u_lightDir[MAX_LIGHTS];       // world direction, normalized (dir/spot)
uniform vec4  u_lightColor[MAX_LIGHTS];     // rgb 0..1
uniform float u_lightIntensity[MAX_LIGHTS];
uniform int   u_lightType[MAX_LIGHTS];      // LightType on the C++ side

out vec4 finalColor;

void main()
{
    // Base color = texture * our color * vertex color.
    vec4 base = texture(texture0, fragTexCoord) * u_baseColor * fragColor;
    vec3 N = normalize(fragNormal);

    // Constant ambient so unlit faces still read; accumulate Lambert per light.
    vec3 lighting = vec3(0.15);

    for (int i = 0; i < u_lightCount; ++i) {
        vec3 L;
        float atten = 1.0;
        if (u_lightType[i] == LIGHT_DIRECTIONAL) {
            L = normalize(-u_lightDir[i]);  // direction points FROM the light
        }
        else {
            // Point (and spot until cone params exist): direction to the light
            // plus inverse-square distance falloff.
            // TODO(spot): cone falloff needs inner/outer angles in LightComponent.
            vec3 toLight = u_lightPos[i] - fragPosition;
            float dist = length(toLight);
            L = toLight / max(dist, 1e-4);
            atten = 1.0 / max(dist * dist, 1e-4);
        }
        float ndl = max(dot(N, L), 0.0);
        lighting += u_lightColor[i].rgb * (ndl * u_lightIntensity[i] * atten);
    }

    // May exceed 1.0 -- fine, the pipeline is HDR (bloom/tonemap handle it).
    finalColor = vec4(base.rgb * lighting, base.a);
}
