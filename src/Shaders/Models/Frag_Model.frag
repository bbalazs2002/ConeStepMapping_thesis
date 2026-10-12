#version 430 core

// This file may contain AI-assisted (Claude Code) code, reviewed by the author.

// pipeline-ból bejövő per-fragment attribútumok
in vec3 vs_out_pos;
in vec3 vs_out_norm;
in vec2 vs_out_tex;

// kimenő érték - a fragment színe
out vec4 fs_out_col;

// camera
#include "../Modules/Camera/Camera_uniforms.glsl"
#include "../Modules/Camera/Camera.glsl"

// material
#include "../Modules/Material/Material_uniforms.glsl"
#include "../Modules/Material/Material.glsl"

void main()
{
    // material
    float[13] colors = MaterialPrepare(vs_out_tex);

    // narmalise vectors
    vec3 norm = normalize(vs_out_norm);
    vec3 viewDir = normalize(cameraData.eye - vs_out_pos);

    fs_out_col = vec4(colors[3] + colors[9], colors[4] + colors[10], colors[5] + colors[11], 1);   // diffuse + emission
}