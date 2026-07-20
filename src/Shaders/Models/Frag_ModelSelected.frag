#version 430

out vec4 fs_out_col;

// color
#include "../Modules/Color/Color_uniforms.glsl"
#include "../Modules/Color/Color.glsl"

void main()
{
	fs_out_col = Color();
}
