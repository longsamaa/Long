#version 430
//BLUR SHADER 
in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0; 

uniform vec2 u_texelDir;      // (1/width, 0) horizontal | (0, 1/height) vertical
uniform int  u_radius;        // blur radius in texels

void main()
{
    // Gaussian weights, normalised by the running total so brightness is preserved.
    vec3 sum = texture(texture0, fragTexCoord).rgb;
    float wsum = 1.0;
    //float sigma = max(float(u_radius), 1.0);
    float sigma = u_radius * 0.5f; 
    for (int i = 1; i <= u_radius; ++i) {
        float w = exp(-float(i * i) / (2.0 * sigma * sigma));
        sum += texture(texture0, fragTexCoord + u_texelDir * float(i)).rgb * w;
        sum += texture(texture0, fragTexCoord - u_texelDir * float(i)).rgb * w;
        wsum += 2.0 * w;
    }
    finalColor = vec4(sum / wsum, 1.0);
}