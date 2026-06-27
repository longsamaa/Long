#version 430

// Bloom mip-chain DOWNSAMPLE (Call of Duty / Jimenez 2014). Takes 13 bilinear
// taps in a hexagonal pattern around the pixel and combines them with weights
// that approximate a wider Gaussian than a naive box filter -- so each half-size
// mip is a clean, low-aliasing reduction of the one above it.

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;   // the mip being downsampled (raylib binds it here)
uniform vec2  u_srcTexel;     // 1 / size of the SOURCE texture (texel step)
uniform int   u_firstPass;    // 1 only on bright -> mip0; applies Karis anti-firefly

vec3 fetch(vec2 uv) { return texture(texture0, uv).rgb; }

// Karis average: weight each group by 1/(1+luma) so a single super-bright pixel
// can't dominate and flicker ("firefly"). Only needed on the first downsample.
float karisWeight(vec3 c) {
    float luma = dot(c, vec3(0.2126, 0.7152, 0.0722));
    return 1.0 / (1.0 + luma);
}

void main()
{
    vec2 t = u_srcTexel;
    vec2 uv = fragTexCoord;

    // 13 taps: center, inner 4 (offset 1 texel diag), outer 8 (offset 2 texels).
    vec3 a = fetch(uv + vec2(-2.0, 2.0) * t);
    vec3 b = fetch(uv + vec2( 0.0, 2.0) * t);
    vec3 c = fetch(uv + vec2( 2.0, 2.0) * t);
    vec3 d = fetch(uv + vec2(-2.0, 0.0) * t);
    vec3 e = fetch(uv + vec2( 0.0, 0.0) * t);
    vec3 f = fetch(uv + vec2( 2.0, 0.0) * t);
    vec3 g = fetch(uv + vec2(-2.0,-2.0) * t);
    vec3 h = fetch(uv + vec2( 0.0,-2.0) * t);
    vec3 i = fetch(uv + vec2( 2.0,-2.0) * t);
    vec3 j = fetch(uv + vec2(-1.0, 1.0) * t);
    vec3 k = fetch(uv + vec2( 1.0, 1.0) * t);
    vec3 l = fetch(uv + vec2(-1.0,-1.0) * t);
    vec3 m = fetch(uv + vec2( 1.0,-1.0) * t);

    if (u_firstPass == 1) {
        // 5 overlapping 2x2 boxes, each Karis-weighted, then combined. This is the
        // standard firefly-suppressing first downsample.
        vec3 g0 = (j + k + l + m) * 0.25;            // center box
        vec3 g1 = (a + b + d + e) * 0.25;            // top-left
        vec3 g2 = (b + c + e + f) * 0.25;            // top-right
        vec3 g3 = (d + e + g + h) * 0.25;            // bottom-left
        vec3 g4 = (e + f + h + i) * 0.25;            // bottom-right
        float w0 = karisWeight(g0) * 0.5;
        float w1 = karisWeight(g1) * 0.125;
        float w2 = karisWeight(g2) * 0.125;
        float w3 = karisWeight(g3) * 0.125;
        float w4 = karisWeight(g4) * 0.125;
        vec3 sum = g0*w0 + g1*w1 + g2*w2 + g3*w3 + g4*w4;
        sum /= (w0 + w1 + w2 + w3 + w4);
        finalColor = vec4(sum, 1.0);
        return;
    }

    // Standard weighted 13-tap combine (weights sum to 1).
    vec3 sum = e * 0.125;
    sum += (a + c + g + i) * 0.03125;
    sum += (b + d + f + h) * 0.0625;
    sum += (j + k + l + m) * 0.125;
    finalColor = vec4(sum, 1.0);
}
