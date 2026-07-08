#version 430

// Debug lines: plain vertex color, no lighting.
in vec4 fragColor;
out vec4 finalColor;

void main()
{
    finalColor = fragColor;
}
