#version 430 core

// stepCount instances × 3 vertices, GL_LINE_STRIP (indirect instanced draw)
// Per step, a V-shape: left → apex → right (2 segments, no duplicate apex)
//   part 0 → left arm endpoint
//   part 1 → cone apex
//   part 2 → right arm endpoint
//
// Cone in texture space: apex = (xy, h), arm endpoints = (xy ± R·uvDir, z)
//   R = (z − h) * tan_val   (tan_val = cot(slope angle) = UV_dist/Δh from conemap)

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
    int step = gl_InstanceID;  // one instance per cone step
    int part = gl_VertexID;   // 0=left, 1=apex, 2=right

    vec4 num = debugNumerical[23 + step * 2];      // (ti, t, height, tan)
    vec4 pos = debugNumerical[23 + step * 2 + 1];  // (x, y, z, 0)

    float h       = num.z;
    float tan_val = num.w;
    vec3  center  = pos.xyz;

    if (tan_val < 1e-6) {
        // In LS mode tan=0 — no cone, emit degenerate vertex
        gl_Position = vec4(0.0);
        v_col       = vec3(0.0);
        return;
    }

    float R      = (center.z - h) * tan_val;  // safe radius in UV space
    // tan_val = min(UV_dist/Δh) = cot(slope angle) → R = Δz * cot(α)

    vec3 rayDir  = debugNumerical[15].xyz;    // TexDir
    vec2 uvDir   = length(rayDir.xy) > 1e-6
                   ? normalize(rayDir.xy)
                   : vec2(1.0, 0.0);

    vec3 apex      = vec3(center.xy,             h);        // surface height = apex
    vec3 left_end  = vec3(center.xy - uvDir * R, center.z); // left arm endpoint
    vec3 right_end = vec3(center.xy + uvDir * R, center.z); // right arm endpoint

    mat4 invM = mat4(debugVisual[1], debugVisual[2], debugVisual[3], debugVisual[4]);

    vec3 texPos;
    if      (part == 0) texPos = left_end;
    else if (part == 1) texPos = apex;
    else                texPos = right_end;

    gl_Position = u_viewProj * invM * vec4(texPos, 1.0);
    v_col       = vec3(0.0, 1.0, 1.0); // cyan
}
