#version 430

// From the vertex shader.
in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragNormal;

uniform sampler2D texture0;   // diffuse/albedo map (raylib sets this)

// Our OWN uniform -- raylib does not touch it, so DefaultMaterial controls it
// fully via SetUniform/ApplyUniforms.
uniform vec4 u_baseColor;

out vec4 finalColor;

void main()
{
    // Base color = texture * our color * vertex color.
    vec4 base = texture(texture0, fragTexCoord) * u_baseColor * fragColor;

    // Simple hard-coded directional light so geometry reads as 3D.
    vec3 lightDir = normalize(vec3(-0.5, -1.0, -0.3));
    float diff = max(dot(normalize(fragNormal), -lightDir), 0.0);
    float ambient = 0.3;
    float light = ambient + diff * 0.7;

    finalColor = vec4(base.rgb * light, base.a);
}
