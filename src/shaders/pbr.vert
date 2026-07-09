#version 430

// Same vertex setup as default.vert: world-space position/normal for the PBR
// BRDF, and both single + instanced paths so PBR meshes batch like the rest.
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;

in mat4 instanceTransform; // per-instance model (instanced path only)

uniform mat4 mvp;       // model-view-projection (non-instanced) / view-proj (instanced)
uniform mat4 matModel;  // model matrix (non-instanced only)

#if defined(SKINNED)
// Bone attributes (locations 7/8 match GLRenderer's kBoneIds/WeightsLoc; loc 6
// is the element index buffer). Ids are passed as float, rounded to int here.
layout(location = 7) in vec4 boneIds;
layout(location = 8) in vec4 boneWeights;
#define MAX_JOINTS 128
uniform mat4 u_jointMatrices[MAX_JOINTS]; // = inverseBind * jointWorld (SkinningSystem)
#endif

out vec2 fragTexCoord;
out vec4 fragColor;
out vec3 fragNormal;
out vec3 fragPosition;  // world-space

void main()
{
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;

#if defined(SKINNED)
    // Weighted blend of the 4 influencing joint matrices -> skinned local pos.
    ivec4 j = ivec4(boneIds + 0.5);
    mat4 skin =
        boneWeights.x * u_jointMatrices[j.x] +
        boneWeights.y * u_jointMatrices[j.y] +
        boneWeights.z * u_jointMatrices[j.z] +
        boneWeights.w * u_jointMatrices[j.w];
    vec4 skinnedPos = skin * vec4(vertexPosition, 1.0);
    vec3 skinnedNormal = mat3(skin) * vertexNormal;
    // matModel places the (already skinned) mesh in the world; mvp includes it.
    fragNormal = normalize(mat3(matModel) * skinnedNormal);
    fragPosition = (matModel * skinnedPos).xyz;
    gl_Position = mvp * skinnedPos;
#elif defined(INSTANCED)
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
