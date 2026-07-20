#version 430 core

// 8 vertices, GL_LINES: 1 cross at the hit point
// If no hit (wasHit = false): degenerate vertices at origin
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
    bool wasHit    = debugNumerical[16].w > 0.5;
    int  stepCount = int(indirectSteps.x);

    if (!wasHit || stepCount == 0) {
        gl_Position = vec4(0.0);
        v_col       = vec3(0.0);
        return;
    }

    mat4 invM   = mat4(debugVisual[1], debugVisual[2], debugVisual[3], debugVisual[4]);
    vec3 hitTex = debugNumerical[23 + (stepCount - 1) * 2 + 1].xyz; // last step position

    int  lineIdx = gl_VertexID / 2;      // 0..3 (which arm)
    bool isEnd   = (gl_VertexID % 2) == 1;

    vec4 centerClip = u_viewProj * invM * vec4(hitTex, 1.0);

    // Screen-space arms: offset in NDC units, scaled by w to survive perspective division
    const float ARM = 0.03;
    vec2 arms[4] = vec2[4](
        vec2( ARM, 0.0),
        vec2(-ARM, 0.0),
        vec2(0.0,  ARM),
        vec2(0.0, -ARM)
    );

    vec4 offset = isEnd ? vec4(arms[lineIdx] * centerClip.w, 0.0, 0.0) : vec4(0.0);
    gl_Position = centerClip + offset;
    v_col = vec3(1.0, 0.2, 0.2); // red
}
