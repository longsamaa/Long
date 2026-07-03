#version 430

// Depth-only pass from the light's point of view. Transforms vertices by the
// light-space matrix (view*proj of the light) so the FBO's depth buffer records
// distance-from-light. Supports both single and instanced draws, matching
// default.vert so the same meshes render.
in vec3 vertexPosition;
in mat4 instanceTransform; // per-instance model (instanced path only)

uniform mat4 mvp;      // non-instanced: lightViewProj * model
uniform mat4 matModel; // model (unused non-instanced -- mvp already has it)

void main()
{
#if defined(INSTANCED)
    // Instanced: mvp = lightViewProj only; model comes from the attribute.
    gl_Position = mvp * instanceTransform * vec4(vertexPosition, 1.0);
#else
    gl_Position = mvp * vec4(vertexPosition, 1.0);
#endif
}
