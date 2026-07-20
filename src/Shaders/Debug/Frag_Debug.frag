#version 430 core

in  vec3 v_col;
out vec4 fs_out_col;

void main()
{
    fs_out_col = vec4(v_col, 1.0);
}
