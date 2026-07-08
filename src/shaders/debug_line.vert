#version 430

// Debug line rendering (grid / camera / light helpers). One interleaved buffer:
// position (3 floats) + color (4 normalized bytes). raylib binds the attribute
// names below to its default locations (vertexPosition=0, vertexColor=3), which
// GLRenderer::DrawDebugLines matches when it builds the VAO.
in vec3 vertexPosition;
in vec4 vertexColor;

uniform mat4 mvp;

out vec4 fragColor;

void main()
{
    fragColor = vertexColor;
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
