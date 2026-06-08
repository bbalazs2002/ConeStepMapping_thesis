#version 430

// pipeline-ból bejövõ per-fragment attribútumok
in vec3 vs_out_color;

// kimenõ érték - a fragment színe
out vec4 fs_out_col;

void main()
{
	fs_out_col = vec4( vs_out_color, 1.0 );
}
