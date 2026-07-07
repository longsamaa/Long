#version 430

// Point-light shadow cube: store LINEAR distance-from-light, normalized to
// [0,1] by the light's range, instead of the rasterizer's non-linear depth.
// The main shader then compares length(fragPos - lightPos)/range against the
// value fetched from the cube by that same direction -- no per-face matrix,
// no depth linearization needed.
in vec3 fragWorldPos;

uniform vec3  u_lightPos;   // world-space position of the point light
uniform float u_lightRange; // must match the far plane used to build the faces

void main()
{
    float dist = length(fragWorldPos - u_lightPos) / max(u_lightRange, 1e-4);
    gl_FragDepth = clamp(dist, 0.0, 1.0); // written to the cube face's depth
}
