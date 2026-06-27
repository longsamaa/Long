#version 430

// Combined tonemap + (optional) FXAA in one pass, saving a full-res read/write vs
// running them separately. FXAA works on perceived LDR luma, so every sample is
// tonemapped first via sampleLDR(). u_fxaaEnabled toggles the antialiasing at
// runtime (game-settings style): when off, it's just a single tonemap fetch.

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;   // composited HDR image
uniform vec2  u_resolution;   // screen size in pixels
uniform float u_exposure;     // brightness before tonemapping
uniform int   u_fxaaEnabled;  // 0 = tonemap only, 1 = tonemap + FXAA

vec3 acesToneMap(vec3 x) {
    const float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

// Sample HDR -> tonemapped, gamma-corrected LDR.
vec3 sampleLDR(vec2 uv) {
    vec3 hdr = texture(texture0, uv).rgb * u_exposure;
    return pow(acesToneMap(hdr), vec3(1.0 / 2.2));
}

const float SPAN_MAX   = 8.0;
const float REDUCE_MUL = 1.0 / 8.0;
const float REDUCE_MIN = 1.0 / 128.0;

float luma(vec3 c) { return dot(c, vec3(0.299, 0.587, 0.114)); }

void main()
{
    // FXAA off: one tonemap fetch, done. Cheapest path.
    if (u_fxaaEnabled == 0) {
        finalColor = vec4(sampleLDR(fragTexCoord), 1.0);
        return;
    }

    vec2 inv = 1.0 / u_resolution;

    vec3 rgbM  = sampleLDR(fragTexCoord);
    vec3 rgbNW = sampleLDR(fragTexCoord + vec2(-1.0, -1.0) * inv);
    vec3 rgbNE = sampleLDR(fragTexCoord + vec2( 1.0, -1.0) * inv);
    vec3 rgbSW = sampleLDR(fragTexCoord + vec2(-1.0,  1.0) * inv);
    vec3 rgbSE = sampleLDR(fragTexCoord + vec2( 1.0,  1.0) * inv);

    float lM  = luma(rgbM);
    float lNW = luma(rgbNW);
    float lNE = luma(rgbNE);
    float lSW = luma(rgbSW);
    float lSE = luma(rgbSE);

    float lMin = min(lM, min(min(lNW, lNE), min(lSW, lSE)));
    float lMax = max(lM, max(max(lNW, lNE), max(lSW, lSE)));

    vec2 dir;
    dir.x = -((lNW + lNE) - (lSW + lSE));
    dir.y =  ((lNW + lSW) - (lNE + lSE));

    float reduce = max((lNW + lNE + lSW + lSE) * 0.25 * REDUCE_MUL, REDUCE_MIN);
    float rcpMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + reduce);
    dir = clamp(dir * rcpMin, vec2(-SPAN_MAX), vec2(SPAN_MAX)) * inv;

    vec3 rgbA = 0.5 * (
        sampleLDR(fragTexCoord + dir * (1.0/3.0 - 0.5)) +
        sampleLDR(fragTexCoord + dir * (2.0/3.0 - 0.5)));
    vec3 rgbB = rgbA * 0.5 + 0.25 * (
        sampleLDR(fragTexCoord + dir * -0.5) +
        sampleLDR(fragTexCoord + dir *  0.5));

    float lB = luma(rgbB);
    finalColor = vec4((lB < lMin || lB > lMax) ? rgbA : rgbB, 1.0);
}
