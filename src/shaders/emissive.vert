#version 430

// Same vertex setup as the other materials. Supports normal and instanced draws
// so an emissive material can be batched too.
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;

in mat4 instanceTransform; // per-instance model (instanced path only)

uniform mat4 mvp;
uniform mat4 matModel;

out vec2 fragTexCoord;
out vec4 fragColor;

void main()
{
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;

#if defined(INSTANCED)
    gl_Position = mvp * instanceTransform * vec4(vertexPosition, 1.0);
#else
    gl_Position = mvp * vec4(vertexPosition, 1.0);
#endif
}
