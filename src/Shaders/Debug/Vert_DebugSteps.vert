#version 430 core

// stepCount vertices, GL_POINTS (indirect draw)
// debugVisual[9 + gl_VertexID] = vec4(uᵢ, 1.0) — step position in texture space

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

    vec3 texPos = debugVisual[9 + gl_VertexID].xyz;
    gl_Position = u_viewProj * invM * vec4(texPos, 1.0);

    // Color: green to yellow toward the last step — makes the march direction visible
    int   stepCount = int(indirectSteps.x);
    float t = (stepCount > 1) ? float(gl_VertexID) / float(stepCount - 1) : 1.0;
    v_col = mix(vec3(0.1, 0.9, 0.1), vec3(0.9, 0.9, 0.1), t);
}
