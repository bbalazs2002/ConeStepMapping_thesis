#version 430 core

// GS is the only stage that writes SSBO debug data
#define DEBUG_FUNCTIONS

layout(triangles) in;
layout(triangle_strip, max_vertices = 18) out;

in vec2 vs_out_tex[];
in vec3 vs_out_norm[];
in vec3 vs_out_merged[];

out vec3 gs_out_tex;
out vec3 gs_out_norm;
out vec3 gs_out_merged;
out vec3 gs_out_pos;
out mat4 gs_out_Scene2Tex;
out vec3 gs_out_TexEye;
out mat4 gs_out_Scene2Unit;
out vec3 gs_out_UnitEye;
out mat3x2 gs_out_triangle;

uniform float normalMult = 0.5;

// Required by RayMarch_common.glsl
layout(binding = 4) uniform sampler2D coneMap;
uniform int maxSteps = 64;

#include "../Modules/Camera/Camera_uniforms.glsl"
#include "RayMarch_common.glsl"

void main()
{
    // -- Common geometry setup -------------------------------------------------
    vec4 verteces[6] = vec4[6](
        gl_in[0].gl_Position,
        gl_in[1].gl_Position,
        gl_in[2].gl_Position,
        gl_in[0].gl_Position + vec4(vs_out_merged[0] * normalMult, 0.0),
        gl_in[1].gl_Position + vec4(vs_out_merged[1] * normalMult, 0.0),
        gl_in[2].gl_Position + vec4(vs_out_merged[2] * normalMult, 0.0)
    );

    vec4 texPos[6] = vec4[6](
        vec4(vs_out_tex[0], 0.0, 1.0),
        vec4(vs_out_tex[1], 0.0, 1.0),
        vec4(vs_out_tex[2], 0.0, 1.0),
        vec4(vs_out_tex[0], 1.0, 1.0),
        vec4(vs_out_tex[1], 1.0, 1.0),
        vec4(vs_out_tex[2], 1.0, 1.0)
    );

    vec4 uprismPos[4] = vec4[4](
        vec4(0.0, 0.0, 0.0, 1.0),
        vec4(1.0, 0.0, 0.0, 1.0),
        vec4(0.0, 0.0, 1.0, 1.0),
        vec4(0.0, 1.0, 0.0, 1.0)
    );

    // M = [u0 u1 u2 u3] * [p0 p1 p2 p3]^-1  (scene → texture space)
    // T = [v0 v1 v2 v3] * [p0 p1 p2 p3]^-1  (scene → unit prism space)
    mat4 uMat = mat4(texPos[0],    texPos[1],    texPos[2],    texPos[3]);
    mat4 pMat = mat4(verteces[0],  verteces[1],  verteces[2],  verteces[3]);
    mat4 vMat = mat4(uprismPos[0], uprismPos[1], uprismPos[2], uprismPos[3]);
    mat4 invP = inverse(pMat);
    mat4 M    = uMat * invP;
    mat4 T    = vMat * invP;

    // -- Debug intersection (once per primitive) -------------------------------
    // Reads config from debugVisualSSBO[5-6] (CPU-written before this draw call).
    // Writes step positions to debugVisualSSBO[9+] and numerical data to
    // debugNumericalSSBO.debugNumerical[17+].
    // Also writes indirect draw commands to indirectSteps and indirectCones.
#ifdef DEBUG_FUNCTIONS
    {
        int showDebug = int(debugVisual[5].x + 0.5);
        int cfgPrimID = int(debugVisual[6].z);

        if (showDebug != 0 && (cfgPrimID < 0 || gl_PrimitiveIDIn == cfgPrimID)) {
            vec3 dbgEye = debugNumerical[1].xyz;
            vec3 dbgAt  = debugNumerical[2].xyz;
            vec3 dbgDir = normalize(dbgAt - dbgEye);

            // Transform debug ray to unit prism space
            vec3 UnitEye = (T * vec4(dbgEye, 1.0)).xyz;
            vec3 UnitAt  = (T * vec4(dbgAt,  1.0)).xyz;
            vec3 UnitDir = normalize(UnitAt - UnitEye);

            UnitIntersection uiInt = intersectUnitPrism(Ray(UnitEye, UnitDir));
            if (uiInt.found) {
                mat4 M_invT    = M * inverse(T);
                vec3 TexEye    = (M * vec4(dbgEye, 1.0)).xyz;

                vec3 UnitEnter = UnitEye + uiInt.near * UnitDir;
                vec3 UnitExit  = UnitEye + uiInt.far  * UnitDir;
                vec3 TexEnter  = (uiInt.near < 0.0) ? TexEye : (M_invT * vec4(UnitEnter, 1.0)).xyz;
                vec3 TexExit   = (M_invT * vec4(UnitExit, 1.0)).xyz;

                g_debugStepIdx = 0;

                int technique = int(debugVisual[6].w + 0.5);
                IntersectReturn result;
                if (technique == 1) {
                    result = findIntersection_coneStepMapping(IntersectParams(TexEnter, TexExit, TexEye));
                } else {
                    result = findIntersection_linearSearch(IntersectParams(TexEnter, TexExit, TexEye));
                }

                // Indirect draw commands (no barrier needed — initialized safely in InitDebugSSBOs)
                indirectSteps = uvec4(uint(g_debugStepIdx), 1u, 0u, 0u);
                indirectCones = uvec4(3u, uint(g_debugStepIdx), 0u, 0u);

                // Write metadata to visual SSBO
                debugVisual[0] = vec4(float(g_debugStepIdx), 0.0, 0.0, 0.0);
                mat4 invM = inverse(M);
                debugVisual[1] = invM[0];
                debugVisual[2] = invM[1];
                debugVisual[3] = invM[2];
                debugVisual[4] = invM[3];

                // Write metadata to numerical SSBO
                debugNumerical[0]  = vec4(float(g_debugStepIdx), float(result.flags), 0.0, 0.0);
                debugNumerical[3]  = M[0];
                debugNumerical[4]  = M[1];
                debugNumerical[5]  = M[2];
                debugNumerical[6]  = M[3];
                debugNumerical[7]  = vec4(TexEye, 0.0);
                debugNumerical[8]  = T[0];
                debugNumerical[9]  = T[1];
                debugNumerical[10] = T[2];
                debugNumerical[11] = T[3];
                debugNumerical[12] = vec4((T * vec4(dbgEye, 1.0)).xyz, 0.0);
                debugNumerical[13] = vec4(TexEnter, 0.0);
                debugNumerical[14] = vec4(TexExit,  0.0);
                debugNumerical[15] = vec4(normalize(TexExit - TexEnter), 0.0);
                debugNumerical[16] = result.wasHit ? vec4(result.uv, 0.0, 1.0) : vec4(0.0);

                // Primitive vertices: scene-space positions and UV coordinates
                // slots [17..19] = vertex[0..2] scene xyz,  slots [20..22] = vertex[0..2] UV
                debugNumerical[17] = vec4(verteces[0].xyz, 0.0);
                debugNumerical[18] = vec4(verteces[1].xyz, 0.0);
                debugNumerical[19] = vec4(verteces[2].xyz, 0.0);
                debugNumerical[20] = vec4(texPos[0].xy, 0.0, 0.0);
                debugNumerical[21] = vec4(texPos[1].xy, 0.0, 0.0);
                debugNumerical[22] = vec4(texPos[2].xy, 0.0, 0.0);
            }
        }
    }
#endif

    // -- Normal prism emission -------------------------------------------------

    gs_out_triangle = mat3x2(
        texPos[0].xy,
        texPos[1].xy,
        texPos[2].xy
    );

    gs_out_Scene2Tex  = M;
    gs_out_TexEye     = (M * vec4(cameraData.eye, 1.0)).xyz;
    gs_out_Scene2Unit = T;
    gs_out_UnitEye    = (T * vec4(cameraData.eye, 1.0)).xyz;

    // -- Bottom face --
    for (int i = 0; i < 3; ++i) {
        gs_out_tex     = texPos[i].xyz;
        gs_out_norm    = -vs_out_norm[i];
        gs_out_merged  = -vs_out_merged[i];
        gs_out_pos     = verteces[i].xyz;
        gl_Position    = cameraData.viewProj * verteces[i];
        EmitVertex();
    }
    EndPrimitive();

    // -- Top face --
    for (int i = 0; i < 3; ++i) {
        gs_out_tex     = texPos[i + 3].xyz;
        gs_out_norm    = vs_out_norm[i];
        gs_out_merged  = vs_out_merged[i];
        gs_out_pos     = verteces[i + 3].xyz;
        gl_Position    = cameraData.viewProj * verteces[i + 3];
        EmitVertex();
    }
    EndPrimitive();

    // -- Side 1 (edge a-b) --
    vec3 norm1 = normalize(cross(
        verteces[0].xyz - verteces[1].xyz,
        verteces[0].xyz - verteces[3].xyz));
    gs_out_norm   = norm1;
    gs_out_merged = norm1;
    gs_out_tex    = texPos[0].xyz; gs_out_pos = verteces[0].xyz; gl_Position = cameraData.viewProj * verteces[0]; EmitVertex();
    gs_out_tex    = texPos[1].xyz; gs_out_pos = verteces[1].xyz; gl_Position = cameraData.viewProj * verteces[1]; EmitVertex();
    gs_out_tex    = texPos[3].xyz; gs_out_pos = verteces[3].xyz; gl_Position = cameraData.viewProj * verteces[3]; EmitVertex();
    gs_out_tex    = texPos[4].xyz; gs_out_pos = verteces[4].xyz; gl_Position = cameraData.viewProj * verteces[4]; EmitVertex();
    EndPrimitive();

    // -- Side 2 (edge b-c) --
    vec3 norm2 = normalize(cross(
        verteces[1].xyz - verteces[4].xyz,
        verteces[2].xyz - verteces[1].xyz));
    gs_out_norm   = norm2;
    gs_out_merged = norm2;
    gs_out_tex    = texPos[1].xyz; gs_out_pos = verteces[1].xyz; gl_Position = cameraData.viewProj * verteces[1]; EmitVertex();
    gs_out_tex    = texPos[2].xyz; gs_out_pos = verteces[2].xyz; gl_Position = cameraData.viewProj * verteces[2]; EmitVertex();
    gs_out_tex    = texPos[4].xyz; gs_out_pos = verteces[4].xyz; gl_Position = cameraData.viewProj * verteces[4]; EmitVertex();
    gs_out_tex    = texPos[5].xyz; gs_out_pos = verteces[5].xyz; gl_Position = cameraData.viewProj * verteces[5]; EmitVertex();
    EndPrimitive();

    // -- Side 3 (edge c-a) --
    vec3 norm3 = normalize(cross(
        verteces[0].xyz - verteces[2].xyz,
        verteces[5].xyz - verteces[2].xyz));
    gs_out_norm   = norm3;
    gs_out_merged = norm3;
    gs_out_tex    = texPos[2].xyz; gs_out_pos = verteces[2].xyz; gl_Position = cameraData.viewProj * verteces[2]; EmitVertex();
    gs_out_tex    = texPos[0].xyz; gs_out_pos = verteces[0].xyz; gl_Position = cameraData.viewProj * verteces[0]; EmitVertex();
    gs_out_tex    = texPos[5].xyz; gs_out_pos = verteces[5].xyz; gl_Position = cameraData.viewProj * verteces[5]; EmitVertex();
    gs_out_tex    = texPos[3].xyz; gs_out_pos = verteces[3].xyz; gl_Position = cameraData.viewProj * verteces[3]; EmitVertex();
    EndPrimitive();
}
