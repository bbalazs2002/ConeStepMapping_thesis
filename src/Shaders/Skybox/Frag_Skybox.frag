#version 430

// pipeline-ból bejövő per-fragment attribútumok
in vec3 vs_out_pos;

out vec4 fs_out_col;

// skybox textúra
uniform samplerCube skyboxTexture;

void main()
{
	fs_out_col = texture(skyboxTexture, vs_out_pos);
}
