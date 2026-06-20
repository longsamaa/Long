#version 430

in vec2 fragTexCoord;

out vec4 finalColor;

uniform sampler2D texture0; // selection mask (selected = non-black, else black)

uniform int size;     // outline thickness in texels
uniform int width;    // mask width  (texels)
uniform int height;   // mask height (texels)
uniform vec4 color;   // outline color

// Is this texel inside the selection?
float inside(vec2 uv) {
    return texture(texture0, uv).r > 0.5 ? 1.0 : 0.0;
}

void main()
{
    float x = 1.0 / float(width);
    float y = 1.0 / float(height);
    float self = inside(fragTexCoord);
    // Only pixels OUTSIDE the selection can become outline.
    if (self > 0.5) {
        discard;
    }
    // Count how many neighbours are inside -> smooth coverage instead of a hard
    // on/off test. This anti-aliases the outline edge.
    float hits = 0.0;
    float total = 0.0;
    for (int i = -size; i <= size; ++i) {
        for (int j = -size; j <= size; ++j) {
            if (i == 0 && j == 0) continue;
            hits  += inside(fragTexCoord + vec2(i, j) * vec2(x, y));
            total += 1.0;
        }
    }
    float coverage = hits / total;       // 0 = no neighbour inside, 1 = all
    if (coverage <= 0.0) {
        discard;                         // not near the silhouette
    }
    float a = clamp(coverage * 3.0, 0.0, 1.0) * color.a;
    finalColor = vec4(color.rgb, a);
}
