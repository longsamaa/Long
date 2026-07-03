#version 430

// Same vertex setup as default.vert. Supports both normal and instanced draws
// so a wireframe material can be batched too.
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;

in mat4 instanceTransform; // per-instance model (instanced path only)

uniform mat4 mvp;
uniform mat4 matModel;

out vec2 fragTexCoord;
out vec4 fragColor;
out vec3 fragNormal;
out vec3 fragPosition;  // world-space (point/spot lights need it)

void main()
{
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;

#if defined(INSTANCED)
    mat4 model = instanceTransform;
    fragNormal = normalize(mat3(model) * vertexNormal);
    fragPosition = (model * vec4(vertexPosition, 1.0)).xyz;
    gl_Position = mvp * model * vec4(vertexPosition, 1.0);
#else
    fragNormal = normalize(mat3(matModel) * vertexNormal);
    fragPosition = (matModel * vec4(vertexPosition, 1.0)).xyz;
    gl_Position = mvp * vec4(vertexPosition, 1.0);
#endif
}
