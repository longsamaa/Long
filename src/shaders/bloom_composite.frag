#version 430

// Bloom composite (HDR -> HDR). Adds the blurred bloom onto the sharp scene and
// keeps the result in HDR. NO tonemapping here on purpose: tonemap must happen
// exactly once, at the very end, after every HDR effect (bloom, god rays, lens
// flare, ...) has been added. Doing it per-effect would compress the range
// multiple times and wash out the colors.

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;   // scene HDR (raylib binds the drawn texture here)
uniform sampler2D u_bloom;    // blurred bloom (second sampler, bound manually)

uniform float u_bloomStrength; // how much bloom to add (e.g. 1.0)

void main()
{
    vec3 scene = texture(texture0, fragTexCoord).rgb;
    // The scene is drawn with a flipped src rect (GL render textures are Y-inverted)
    // so its UVs come in already flipped; u_bloom is sampled directly, so flip Y
    // here to line the bloom up with the scene.
    vec2 bloomUV = vec2(fragTexCoord.x, fragTexCoord.y);
    vec3 bloom = texture(u_bloom, bloomUV).rgb;
    // Additive: bright areas spill light onto their surroundings. Still HDR.
    finalColor = vec4(scene + bloom * u_bloomStrength, 1.0);
}
