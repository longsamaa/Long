#version 430

in vec3 fragPosition;
out vec4 finalColor;

uniform sampler2D equirectangularMap; // the panorama
// Convert a 3D direction to equirectangular UV (longitude/latitude mapping).
const vec2 invAtan = vec2(0.1591, 0.3183); // 1/(2*PI), 1/PI
vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main()
{
    vec2 uv = SampleSphericalMap(normalize(fragPosition));
    finalColor = vec4(texture(equirectangularMap, uv).rgb, 1.0);
}
