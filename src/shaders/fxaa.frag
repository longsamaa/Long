#version 430

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;   // the composited image
uniform vec2 u_resolution;    // screen size in pixels

// Cheap FXAA (Fast Approximate Anti-Aliasing), based on Timothy Lottes' FXAA3.
// Detects edges from luma contrast between neighbours and blurs along the edge.

const float SPAN_MAX   = 8.0;
const float REDUCE_MUL = 1.0 / 8.0;
const float REDUCE_MIN = 1.0 / 128.0;

float luma(vec3 c) { return dot(c, vec3(0.299, 0.587, 0.114)); }

void main()
{
    vec2 inv = 1.0 / u_resolution;

    vec3 rgbM  = texture(texture0, fragTexCoord).rgb;
    vec3 rgbNW = texture(texture0, fragTexCoord + vec2(-1.0, -1.0) * inv).rgb;
    vec3 rgbNE = texture(texture0, fragTexCoord + vec2( 1.0, -1.0) * inv).rgb;
    vec3 rgbSW = texture(texture0, fragTexCoord + vec2(-1.0,  1.0) * inv).rgb;
    vec3 rgbSE = texture(texture0, fragTexCoord + vec2( 1.0,  1.0) * inv).rgb;

    float lM  = luma(rgbM);
    float lNW = luma(rgbNW);
    float lNE = luma(rgbNE);
    float lSW = luma(rgbSW);
    float lSE = luma(rgbSE);

    float lMin = min(lM, min(min(lNW, lNE), min(lSW, lSE)));
    float lMax = max(lM, max(max(lNW, lNE), max(lSW, lSE)));

    // Edge direction (perpendicular to luma gradient).
    vec2 dir;
    dir.x = -((lNW + lNE) - (lSW + lSE));
    dir.y =  ((lNW + lSW) - (lNE + lSE));

    float reduce = max((lNW + lNE + lSW + lSE) * 0.25 * REDUCE_MUL, REDUCE_MIN);
    float rcpMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + reduce);
    dir = clamp(dir * rcpMin, vec2(-SPAN_MAX), vec2(SPAN_MAX)) * inv;

    // Sample along the edge and blend.
    vec3 rgbA = 0.5 * (
        texture(texture0, fragTexCoord + dir * (1.0/3.0 - 0.5)).rgb +
        texture(texture0, fragTexCoord + dir * (2.0/3.0 - 0.5)).rgb);
    vec3 rgbB = rgbA * 0.5 + 0.25 * (
        texture(texture0, fragTexCoord + dir * -0.5).rgb +
        texture(texture0, fragTexCoord + dir *  0.5).rgb);

    float lB = luma(rgbB);
    finalColor = vec4((lB < lMin || lB > lMax) ? rgbA : rgbB, 1.0);
}
