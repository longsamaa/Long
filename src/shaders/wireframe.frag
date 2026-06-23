#version 430

// Lit cube (same as default.frag) PLUS a box-edge wireframe on top. Each cube
// face has UV in [0,1], so fragments near a UV border lie on a face edge --
// painting those gives the 12 box edges (no face diagonals).

in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragNormal;

uniform sampler2D texture0;   // diffuse map (raylib sets this)

// Our uniforms (raylib doesn't touch the u_ prefix).
uniform vec4  u_faceColor;  // cube tint (like default's u_baseColor)
uniform vec4  u_lineColor;  // edge color
uniform float u_thickness;  // edge width in UV space (0..0.5), e.g. 0.03

out vec4 finalColor;

void main()
{
    // --- Lit base color (same lighting as default.frag) ---
    vec4 base = texture(texture0, fragTexCoord) * u_faceColor * fragColor;
    vec3 lightDir = normalize(vec3(-0.5, -1.0, -0.3));
    float diff = max(dot(normalize(fragNormal), -lightDir), 0.0);
    float light = 0.3 + diff * 0.7;
    vec3 litFace = base.rgb * light;

    // --- Edge mask from UV distance to the nearest face border ---
    vec2 d = min(fragTexCoord, 1.0 - fragTexCoord);
    float edge = min(d.x, d.y);
    float aa = fwidth(edge);
    float lineMask = 1.0 - smoothstep(u_thickness, u_thickness + aa, edge);

    // Edge color overrides the lit face where the mask is on.
    vec3 rgb = mix(litFace, u_lineColor.rgb, lineMask);
    finalColor = vec4(rgb, base.a);
}
