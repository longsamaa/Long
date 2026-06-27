#version 430

// Bloom mip-chain UPSAMPLE. Reads a smaller mip with a 9-tap tent filter (a 3x3
// weighted box that smooths the magnification) and the result is ADDED onto the
// next-larger mip via additive blending set up on the CPU side. Walking from the
// smallest mip up to the largest accumulates all the blur radii into one soft,
// wide glow.

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;   // the smaller mip being upsampled (raylib binds it)
uniform vec2  u_srcTexel;     // 1 / size of the SOURCE (smaller) mip
uniform float u_radius;       // tent spread in texels (e.g. 1.0)

vec3 fetch(vec2 uv) { return texture(texture0, uv).rgb; }

void main()
{
    vec2 r = u_srcTexel * u_radius;
    vec2 uv = fragTexCoord;

    // 3x3 tent (Pascal) weights: corners 1, edges 2, center 4, total 16.
    vec3 sum  = fetch(uv + vec2(-1.0,  1.0) * r) * 1.0;
    sum += fetch(uv + vec2( 0.0,  1.0) * r) * 2.0;
    sum += fetch(uv + vec2( 1.0,  1.0) * r) * 1.0;
    sum += fetch(uv + vec2(-1.0,  0.0) * r) * 2.0;
    sum += fetch(uv + vec2( 0.0,  0.0) * r) * 4.0;
    sum += fetch(uv + vec2( 1.0,  0.0) * r) * 2.0;
    sum += fetch(uv + vec2(-1.0, -1.0) * r) * 1.0;
    sum += fetch(uv + vec2( 0.0, -1.0) * r) * 2.0;
    sum += fetch(uv + vec2( 1.0, -1.0) * r) * 1.0;
    finalColor = vec4(sum / 16.0, 1.0);
}
