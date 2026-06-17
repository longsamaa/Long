#version 430

// Vertex attributes (names raylib feeds automatically).
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;

// Uniforms raylib sets automatically.
uniform mat4 mvp;       // model-view-projection
uniform mat4 matModel;  // model matrix (for world-space normal/pos)

// Passed to the fragment shader.
out vec2 fragTexCoord;
out vec4 fragColor;
out vec3 fragNormal;

void main()
{
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;
    // World-space normal (assumes uniform scale; fine for now).
    fragNormal = normalize(mat3(matModel) * vertexNormal);

    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
