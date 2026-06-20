#version 430

in vec3 vertexPosition;

uniform mat4 matProjection;
uniform mat4 matView;

out vec3 fragPosition;

void main()
{
    fragPosition = vertexPosition;
    mat4 rotView = mat4(mat3(matView));
    vec4 clip = matProjection * rotView * vec4(vertexPosition, 1.0);
    gl_Position = clip.xyww; // depth = far plane
}
