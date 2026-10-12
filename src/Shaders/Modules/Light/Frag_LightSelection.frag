// This file may contain AI-assisted (Claude Code) code, reviewed by the author.

#version 430

in vec3 vs_out_col;
out vec4 fs_out_col;

void main() {
	fs_out_col = vec4(vs_out_col,1);
	// fs_out_col = Color();
}