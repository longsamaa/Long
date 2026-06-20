#version 430

in vec3 vertexPosition;

uniform mat4 matProjection;
uniform mat4 matView;

out vec3 fragPosition;

void main()
{
    fragPosition = vertexPosition;

    // Strip translation from the view matrix so the skybox stays centered on
    // the camera. raylib sets matView/matProjection for model draws.
    mat4 rotView = mat4(mat3(matView));
    vec4 clip = matProjection * rotView * vec4(vertexPosition, 1.0);
    // .xyww forces z = w so depth is always the FAR plane -> skybox sits behind
    // everything and never covers the scene.
    gl_Position = clip.xyww;
}
