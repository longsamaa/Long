#version 430

// Depth-only pass from the light's point of view. Transforms vertices by the
// light-space matrix (view*proj of the light) so the FBO's depth buffer records
// distance-from-light. Supports both single and instanced draws, matching
// default.vert so the same meshes render.
in vec3 vertexPosition;
in mat4 instanceTransform; // per-instance model (instanced path only)

uniform mat4 mvp;      // non-instanced: lightViewProj * model
uniform mat4 matModel; // model (unused non-instanced -- mvp already has it)

#if defined(SKINNED)
// Same bone attribute layout as pbr.vert (locations 7/8, see GLRenderer).
layout(location = 7) in vec4 boneIds;
layout(location = 8) in vec4 boneWeights;
#define MAX_JOINTS 128
uniform mat4 u_jointMatrices[MAX_JOINTS]; // = jointWorld * inverseBind (world-space)
#endif

void main()
{
#if defined(SKINNED)
    // Skinned: jointMatrices already yield WORLD-space positions, and
    // RenderSystem submits identity as the model matrix, so mvp = lightViewProj.
    ivec4 j = ivec4(boneIds + 0.5);
    mat4 skin =
        boneWeights.x * u_jointMatrices[j.x] +
        boneWeights.y * u_jointMatrices[j.y] +
        boneWeights.z * u_jointMatrices[j.z] +
        boneWeights.w * u_jointMatrices[j.w];
    gl_Position = mvp * (skin * vec4(vertexPosition, 1.0));
#elif defined(INSTANCED)
    // Instanced: mvp = lightViewProj only; model comes from the attribute.
    gl_Position = mvp * instanceTransform * vec4(vertexPosition, 1.0);
#else
    gl_Position = mvp * vec4(vertexPosition, 1.0);
#endif
}
