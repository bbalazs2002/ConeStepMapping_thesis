#version 430 core

layout (location = 0) in vec3 vs_in_pos;
layout (location = 1) in vec3 vs_in_norm;
layout (location = 2) in vec3 vs_in_merged;
layout (location = 3) in vec2 vs_in_tex;

out vec2 vs_out_tex;
out vec3 vs_out_norm;
out vec3 vs_out_merged;

uniform mat4 world;
uniform mat4 worldIT;

void main()
{
    // A normálisokat világ térbe forgatjuk
    vs_out_norm   = normalize(mat3(worldIT) * vs_in_norm).xyz;
    vs_out_merged = normalize(mat3(worldIT) * vs_in_merged).xyz;
    vs_out_tex    = vs_in_tex;

    // A GS-nek a világ koordinátát adjuk át
    gl_Position = world * vec4(vs_in_pos, 1.0);
}