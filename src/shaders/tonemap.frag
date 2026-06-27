#version 430

// Final tonemap pass: HDR -> LDR for display. Runs ONCE, at the very end, after
// every HDR effect has been composited. Maps the open-ended HDR range into [0,1]
// with an ACES filmic curve so highlights roll off to white smoothly instead of
// clipping, then converts linear -> sRGB (gamma) for the screen.

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;   // composited HDR image
uniform float u_exposure;     // overall brightness before tonemapping (e.g. 1.0)

// ACES filmic tonemap (Narkowicz fit).
vec3 acesToneMap(vec3 x) {
    const float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main()
{
    vec3 hdr = texture(texture0, fragTexCoord).rgb * u_exposure;
    vec3 mapped = acesToneMap(hdr);
    mapped = pow(mapped, vec3(1.0 / 2.2)); // linear -> sRGB
    finalColor = vec4(mapped, 1.0);
}
