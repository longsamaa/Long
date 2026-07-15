#version 430

in vec3 vertexPosition;
uniform mat4 mvp;

#if defined(SKINNED)
// Same bone attribute layout as pbr.vert (locations 7/8, see GLRenderer).
layout(location = 7) in vec4 boneIds;
layout(location = 8) in vec4 boneWeights;
// SSBO, binding = GLRenderer's kJointSsboBinding (no joint-count cap).
layout(std430, binding = 3) readonly buffer JointMatricesBlock {
    mat4 u_jointMatrices[]; // world-space skinning matrices
};
#endif

void main()
{
#if defined(SKINNED)
    // Skinned: jointMatrices give world-space directly; the caller draws with an
    // identity model matrix, so mvp = view*proj.
    ivec4 j = ivec4(boneIds + 0.5);
    mat4 skin =
        boneWeights.x * u_jointMatrices[j.x] +
        boneWeights.y * u_jointMatrices[j.y] +
        boneWeights.z * u_jointMatrices[j.z] +
        boneWeights.w * u_jointMatrices[j.w];
    gl_Position = mvp * (skin * vec4(vertexPosition, 1.0));
#else
    gl_Position = mvp * vec4(vertexPosition, 1.0);
#endif
}
