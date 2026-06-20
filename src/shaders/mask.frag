#version 430

out vec4 finalColor;

uniform vec4 u_maskColor;

void main()
{
    finalColor = u_maskColor;
}
