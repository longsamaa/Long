#version 430

// Visualizes a depth texture: samples .r (depth in [0,1]) and shows it as
// grayscale. Depth values cluster near 1.0, so we stretch with a power curve to
// make occluders visible. Debug-only.
in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0; // the depth map (raylib binds the drawn texture here)

out vec4 finalColor;

void main()
{
    float d = texture(texture0, fragTexCoord).r;
    // Stretch: most of the scene sits near far plane; pow pulls detail out.
    float v = pow(d, 20.0);
    finalColor = vec4(vec3(v), 1.0);
}
