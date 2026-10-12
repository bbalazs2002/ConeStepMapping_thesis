#version 430

// This file may contain AI-assisted (Claude Code) code, reviewed by the author.

out vec4 fs_out_col;

// color
#include "../Modules/Color/Color_uniforms.glsl"
#include "../Modules/Color/Color.glsl"

void main()
{
	fs_out_col = Color();
}
