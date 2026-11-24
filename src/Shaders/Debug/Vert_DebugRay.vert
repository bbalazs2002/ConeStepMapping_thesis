#version 430 core

// This file may contain AI-assisted (Claude Code) code, reviewed by the author.

// debugVisualSSBO    (binding 0): slot[1..4] = invM (tex->scene)
// debugNumericalSSBO (binding 1): slot[1] = eye (scene), slot[13] = TexEnter, slot[14] = TexExit

layout(std430, binding = 0) buffer DebugVisualSSBO { vec4 debugVisual[]; };
layout(std430, binding = 1) buffer DebugNumericalSSBO {
    uvec4 indirectSteps;
    uvec4 indirectCones;
    vec4  debugNumerical[];
};

uniform mat4 u_viewProj;
out vec3 v_col;

void main()
{
    mat4 invM = mat4(debugVisual[1], debugVisual[2], debugVisual[3], debugVisual[4]);

    vec3 eye = debugNumerical[1].xyz; // scene space (CPU-written)

    // invM is a genuine projective (not affine) inverse when the apex-based
    // merged-normal correction is active — perspective divide is required
    // here (unlike u_viewProj-chained cases elsewhere, these results feed
    // into a plain position array, not an automatically-divided gl_Position).
    vec4 enterH = invM * vec4(debugNumerical[13].xyz, 1.0); // TexEnter → scene space
    vec4 exitH  = invM * vec4(debugNumerical[14].xyz, 1.0); // TexExit  → scene space
    vec3 enter  = enterH.xyz / enterH.w;
    vec3 exit   = exitH.xyz  / exitH.w;

    // GL_LINES: (0,1) = eye→enter, (2,3) = enter→exit
    vec3 positions[4] = vec3[4](eye, enter, enter, exit);
    gl_Position = u_viewProj * vec4(positions[gl_VertexID], 1.0);
    v_col       = gl_VertexID < 2 ? vec3(0.3, 0.3, 1.0)   // dim blue: eye→enter
                                  : vec3(0.0, 0.6, 1.0);  // bright blue: enter→exit
}
