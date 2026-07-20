#version 430 core

// 16 vertices, GL_LINES: 2 crosses × 4 lines × 2 vertices
// Arms are screen-space offsets (constant NDC size, always face camera).

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

    int  crossIdx = gl_VertexID / 8;     // 0 = Enter, 1 = Exit
    int  localVid = gl_VertexID % 8;     // 0..7
    int  lineIdx  = localVid / 2;        // 0..3 (which arm)
    bool isEnd    = (localVid % 2) == 1; // false = center, true = arm endpoint

    vec3 center = (crossIdx == 0)
        ? debugNumerical[13].xyz   // TexEnter
        : debugNumerical[14].xyz;  // TexExit

    vec4 centerClip = u_viewProj * invM * vec4(center, 1.0);

    // Screen-space arms: offset in NDC units, scaled by w to survive perspective division
    const float ARM = 0.025;
    vec2 arms[4] = vec2[4](
        vec2( ARM, 0.0),
        vec2(-ARM, 0.0),
        vec2(0.0,  ARM),
        vec2(0.0, -ARM)
    );

    vec4 offset = isEnd ? vec4(arms[lineIdx] * centerClip.w, 0.0, 0.0) : vec4(0.0);
    gl_Position = centerClip + offset;
    v_col = vec3(1.0, 1.0, 0.0); // yellow
}
