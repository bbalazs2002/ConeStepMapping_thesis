#version 430 core

// debugVisualSSBO    (binding 0): slot[1..4] = invM (tex→scene)
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

    vec3 eye   = debugNumerical[1].xyz;                           // scene space (CPU-written)
    vec3 enter = (invM * vec4(debugNumerical[13].xyz, 1.0)).xyz;  // TexEnter → scene space
    vec3 exit = (invM * vec4(debugNumerical[14].xyz, 1.0)).xyz;   // TexExit  → scene space

    // GL_LINES: (0,1) = eye→enter, (2,3) = enter→exit
    vec3 positions[4] = vec3[4](eye, enter, enter, exit);
    gl_Position = u_viewProj * vec4(positions[gl_VertexID], 1.0);
    v_col       = gl_VertexID < 2 ? vec3(0.3, 0.3, 1.0)   // dim blue: eye→enter
                                  : vec3(0.0, 0.6, 1.0);  // bright blue: enter→exit
}
