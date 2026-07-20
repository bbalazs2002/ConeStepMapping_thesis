#version 430 core

layout (location = 0) in vec3 vs_in_pos;
layout (location = 1) in vec3 vs_in_norm;
layout (location = 2) in vec3 vs_in_merged;
layout (location = 3) in vec2 vs_in_tex;

out vec2 vs_out_tex;
out vec3 vs_out_norm;
out vec3 vs_out_merged;

#include "../Modules/Transform/Transform_uniforms.glsl"

void main()
{
    mat3 worldIT  = mat3(transpose(inverse(transformData.world)));
    vs_out_norm   = normalize(worldIT * vs_in_norm);
    vs_out_merged = normalize(worldIT * vs_in_merged);
    vs_out_tex    = vs_in_tex;

    // Pass world-space position to geometry shader (no viewProj yet — GS handles it)
    gl_Position = transformData.world * vec4(vs_in_pos, 1.0);
}
