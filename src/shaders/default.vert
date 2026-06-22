#version 430

// Vertex attributes (names raylib feeds automatically).
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;

// Per-instance model matrix. raylib binds this to the attribute named
// "instanceTransform" (SHADER_LOC_VERTEX_INSTANCETRANSFORM) when drawing with
// DrawMeshInstanced. Unused (and harmless) in the non-instanced path.
in mat4 instanceTransform;

// Uniforms raylib sets automatically.
// NOTE: in the instanced path raylib sets mvp = view * projection (NO model),
// and the per-instance model matrix comes from instanceTransform instead.
uniform mat4 mvp;       // model-view-projection (non-instanced) / view-proj (instanced)
uniform mat4 matModel;  // model matrix (non-instanced only)

// Passed to the fragment shader.
out vec2 fragTexCoord;
out vec4 fragColor;
out vec3 fragNormal;

void main()
{
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;

#if defined(INSTANCED)
    // Instanced: model matrix is per-instance; mvp is only view*projection.
    mat4 model = instanceTransform;
    fragNormal = normalize(mat3(model) * vertexNormal);
    gl_Position = mvp * model * vec4(vertexPosition, 1.0);
#else
    // Single draw: mvp already includes the model matrix.
    fragNormal = normalize(mat3(matModel) * vertexNormal);
    gl_Position = mvp * vec4(vertexPosition, 1.0);
#endif
}
