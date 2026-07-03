#version 430

// No color output -- the FBO has no color attachment. Depth is written
// automatically by the rasterizer via gl_FragDepth (default). This shader body
// exists only because GL requires a fragment stage; it does nothing.
void main()
{
}
