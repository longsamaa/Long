#version 430

// Depth pass for a POINT light's shadow cubemap. Unlike shadow_depth.vert
// (directional/spot), this must pass the WORLD-space position to the fragment
// stage so the fragment can write LINEAR distance-to-light instead of the
// default non-linear clip-space depth -- the main shader samples the cube by a
// direction vector and has no way to reconstruct per-face non-linear depth.
//
// `mvp` here is the current face's (view*proj) -- WITHOUT the model matrix in
// the instanced path (model comes from the attribute), WITH it folded in for
// the non-instanced path. We keep matModel separately in both paths to build
// the world position.
in vec3 vertexPosition;
in mat4 instanceTransform; // per-instance model (instanced path only)

uniform mat4 mvp;      // non-instanced: faceViewProj * model ; instanced: faceViewProj
uniform mat4 matModel; // model matrix (non-instanced path)

#if defined(SKINNED)
// Same bone attribute layout as pbr.vert (locations 7/8, see GLRenderer).
layout(location = 7) in vec4 boneIds;
layout(location = 8) in vec4 boneWeights;
// SSBO, binding = GLRenderer's kJointSsboBinding (no joint-count cap).
layout(std430, binding = 3) readonly buffer JointMatricesBlock {
    mat4 u_jointMatrices[]; // world-space skinning matrices
};
#endif

out vec3 fragWorldPos;

void main()
{
#if defined(SKINNED)
    // Skinned: jointMatrices give WORLD-space directly (model is identity),
    // so the skinned position doubles as the world position.
    ivec4 j = ivec4(boneIds + 0.5);
    mat4 skin =
        boneWeights.x * u_jointMatrices[j.x] +
        boneWeights.y * u_jointMatrices[j.y] +
        boneWeights.z * u_jointMatrices[j.z] +
        boneWeights.w * u_jointMatrices[j.w];
    vec4 world = skin * vec4(vertexPosition, 1.0);
    gl_Position = mvp * world;
#elif defined(INSTANCED)
    // Instanced: model is the per-instance attribute; mvp is faceViewProj only.
    vec4 world = instanceTransform * vec4(vertexPosition, 1.0);
    gl_Position = mvp * world;
#else
    // Single draw: mvp already folds in the model matrix for clip position,
    // but we still need matModel to get the world position.
    vec4 world = matModel * vec4(vertexPosition, 1.0);
    gl_Position = mvp * vec4(vertexPosition, 1.0);
#endif
    fragWorldPos = world.xyz;
}
