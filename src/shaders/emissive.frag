#version 430

// Self-illuminating surface. Outputs color * intensity with NO lighting -- the
// intensity can push values well above 1.0, which is exactly what bloom's
// bright-pass looks for. Only meaningful when rendered into an HDR target;
// an LDR target would clamp the result back to 1.0 and there'd be no glow.

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;          // raylib's diffuse (usually white)

uniform vec4  u_emissiveColor;       // base glow color
uniform float u_emissiveIntensity;   // >1.0 -> blooms

out vec4 finalColor;

void main()
{
    vec3 base = texture(texture0, fragTexCoord).rgb * u_emissiveColor.rgb * fragColor.rgb;
    finalColor = vec4(base * u_emissiveIntensity, u_emissiveColor.a);
}
