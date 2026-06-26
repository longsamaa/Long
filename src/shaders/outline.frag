#version 430

// SEPARABLE outline (1D pass). Run twice: horizontal then vertical. Each pass
// dilates the selection coverage along one axis, so two 1D passes (O(size))
// replace the old (2*size+1)^2 box (O(size^2)).
//
// To avoid a second sampler, the original (un-dilated) mask is carried through
// the intermediate texture's GREEN channel while the dilated coverage lives in
// RED. The final pass emits outline where RED (dilated) is set but GREEN
// (original) is not -> the silhouette ring only.

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0; // pass1: raw mask (.r). pass2: temp (.r dilated, .g orig)

uniform int   size;       // outline thickness in texels
uniform int   width;      // texture width  (texels)
uniform int   height;     // texture height (texels)
uniform vec2  direction;  // (1,0) horizontal pass, (0,1) vertical pass
uniform int   isFinal;    // 1 on the vertical pass -> emit the outline color
uniform vec4  color;      // outline color

void main()
{
    vec2 texel = vec2(1.0 / float(width), 1.0 / float(height)) * direction;

    // Dilated coverage = max along this axis within +/- size.
    // For the horizontal pass we read .r of the raw mask; for the vertical pass
    // we read .r of the temp (already horizontally dilated).
    float center = texture(texture0, fragTexCoord).r;
    float cov = center;
    for (int i = 1; i <= size; ++i) {
        cov = max(cov, texture(texture0, fragTexCoord + texel * float(i)).r);
        cov = max(cov, texture(texture0, fragTexCoord - texel * float(i)).r);
    }

    if (isFinal == 0) {
        // Intermediate: R = horizontally-dilated coverage, G = original mask at
        // this exact pixel (carried untouched so the final pass can subtract it).
        finalColor = vec4(cov, center, 0.0, 1.0);
        return;
    }

    // Final pass: orig comes from the temp's GREEN channel (the raw mask). Outline
    // is the dilated region MINUS the original selection.
    //
    // The temp is sampled bilinear (it's lower-res), so `cov` and `orig` come back
    // as smooth 0..1 ramps at edges -> a blurry outline. Threshold both at 0.5 to
    // snap them back to hard edges, restoring a crisp outline despite the half-res
    // dilation. (step() returns 0/1, so the result is fully opaque or discarded.)
    float orig = texture(texture0, fragTexCoord).g;
    float inside = step(0.5, orig);   // 1 if this pixel was in the original selection
    float dilated = step(0.5, cov);   // 1 if inside the thickened silhouette
    float ring = dilated * (1.0 - inside);
    if (ring < 0.5) {
        discard;
    }
    finalColor = vec4(color.rgb, color.a);
}
