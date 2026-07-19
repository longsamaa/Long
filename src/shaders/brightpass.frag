#version 430
//BRIGHT SHADER 
in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;   // HDR scene color (raylib binds the drawn texture here)

uniform float u_threshold;    // brightness above which a pixel blooms (e.g. 1.0)
uniform float u_softKnee;     // 0 = hard cut, ~0.5 = soft roll-off around threshold
uniform float u_clampMax;     // firefly clamp: cap input brightness before thresholding

void main()
{
    vec3 c = texture(texture0, fragTexCoord).rgb;
    // Perceived brightness (Rec. 709 luma -- eyes weight green most).
    float lum = dot(c, vec3(0.2126, 0.7152, 0.0722));
    // Firefly clamp: a tiny mirror-smooth specular highlight can hit HDR values
    // of 50+, which the blur then spreads into a huge glare star. Rescale such
    // pixels down to u_clampMax so hot spots bloom gently instead of exploding.
    if (lum > u_clampMax) {
        c *= u_clampMax / lum;
        lum = u_clampMax;
    }
    // Soft-knee threshold: ramp the contribution smoothly over a "knee" band around
    // u_threshold instead of a hard step, so the bloom edge isn't a harsh line.
    float knee = max(u_threshold * u_softKnee, 0.00001);
    float soft = clamp(lum - u_threshold + knee, 0.0, 2.0 * knee);
    soft = (soft * soft) / (4.0 * knee + 0.00001);
    // Scale (not replace) the color so it keeps its original hue; normalise by luma.
    float contribution = max(soft, lum - u_threshold);
    contribution /= max(lum, 0.00001);
    finalColor = vec4(c * contribution, 1.0);
}
