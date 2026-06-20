#version 430

in vec3 fragPosition;
out vec4 finalColor;

// Two-color sky gradient (our own uniforms).
uniform vec3 u_topColor;     // sky above the horizon
uniform vec3 u_bottomColor;  // ground below the horizon
uniform float u_sharpness;   // 0 = soft blend, 1 = hard horizon line

void main()
{
    vec3 dir = normalize(fragPosition);

    // dir.y: -1 (down) .. 0 (horizon) .. +1 (up). Blend across a band centered
    // on the horizon; a narrow band (high sharpness) gives a clear horizon line.
    float band = mix(0.6, 0.02, clamp(u_sharpness, 0.0, 1.0));
    float t = smoothstep(-band, band, dir.y);

    vec3 col = mix(u_bottomColor, u_topColor, t);
    finalColor = vec4(col, 1.0);
}
