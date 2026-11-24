#version 430 core

// This file may contain AI-assisted (Claude Code) code, reviewed by the author.

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
    #ifdef DEBUG_FUNCTIONS
    // Debug primitive isolation: when a specific primitive is selected for
    // inspection, skip emitting every other primitive's geometry entirely —
    // this makes it practical to visually confirm which triangle a given ID
    // actually is (e.g. a pole-region sliver), instead of hunting for it by
    // eye across the whole model while only its debug overlay is distinct.
    if (int(debugVisual[5].x + 0.5) != 0) {
        int cfgPrimIDEarly = int(debugVisual[6].z);
        if (cfgPrimIDEarly >= 0 && gl_PrimitiveIDIn != cfgPrimIDEarly) {
            return;
        }
    }
    #endif

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

    // -- Scene -> Texture / Unit-prism space transforms -------------------------
    // Apex-centred projective correction for merged (per-vertex-averaged)
    // normals — see docs/ImplementationPlan_MergedNormals.md for the full
    // derivation. Falls back to the original single-corner affine construction
    // whenever the three vertex normals are too close to parallel for a stable
    // apex fit (the case where the affine construction is already accurate).
    mat4 M, T;

    // Mirrors of the apex-fit internals below, hoisted out of the block scope
    // so the debug-write section further down can report them (diagnostic
    // only — M/T construction itself is unaffected by these copies).
    vec3  dbgD        = verteces[3].xyz;
    vec3  dbgE        = verteces[4].xyz;
    vec3  dbgF        = verteces[5].xyz;
    vec3  dbgPApex    = vec3(0.0);
    float dbgMu       = 0.0, dbgNu = 0.0, dbgRho = 0.0;
    float dbgK        = 0.0, dbgWB = 0.0, dbgWC  = 0.0;
    bool  dbgUsedApex = false;

    {
        vec3 a  = verteces[0].xyz, b  = verteces[1].xyz, c  = verteces[2].xyz;
        vec3 na = vs_out_merged[0], nb = vs_out_merged[1], nc = vs_out_merged[2];

        // Least-squares closest point to the three vertex-normal lines.
        mat3 Ma = mat3(1.0) - outerProduct(na, na);
        mat3 Mb = mat3(1.0) - outerProduct(nb, nb);
        mat3 Mc = mat3(1.0) - outerProduct(nc, nc);
        mat3 S  = Ma + Mb + Mc;

        bool  useApex = abs(determinant(S)) >= 1e-5;
        float k = 0.0, wB = 0.0, wC = 0.0;
        vec3  P_apex, Acol, Bcol, Ccol;
        mat3  Linv;

        if (useApex) {
            vec3 d = verteces[3].xyz, e = verteces[4].xyz, f = verteces[5].xyz;

            vec3 rhs = Ma * a + Mb * b + Mc * c;
            P_apex   = inverse(S) * rhs;

            Acol   = a - P_apex; Bcol = b - P_apex; Ccol = c - P_apex;
            mat3 L = mat3(Acol, Bcol, Ccol);
            Linv   = inverse(L);

            // mu, nu, rho: radial position of d, e, f relative to a, b, c, seen
            // from the apex (projected onto each ray — the least-squares apex
            // generally does not sit exactly on all three normal lines).
            float mu  = dot(d - P_apex, normalize(Acol)) / length(Acol);
            float nu  = dot(e - P_apex, normalize(Bcol)) / length(Bcol);
            float rho = dot(f - P_apex, normalize(Ccol)) / length(Ccol);

            k  = mu / (mu - 1.0);
            wB = k * (nu  - 1.0) / nu;
            wC = k * (rho - 1.0) / rho;

            // Ill-conditioned apex fit: mu, nu or rho too close to 1 (or fallen
            // outside the "outward extrusion" sign pattern altogether — sliver
            // triangles, e.g. near UV-sphere poles/seams, can produce this) send
            // k, wB or wC non-positive or blown up in magnitude. Since M and T
            // share the same homogeneous denominator D = alpha + wB*beta +
            // wC*gamma (R's and R_M's W row), a non-positive wB/wC lets D cross
            // zero inside the physically searched region — corrupting both the
            // texture mapping and the entry/exit bound the march relies on to
            // stop at the prism boundary. Fall back to the affine construction
            // instead of risking that.
            useApex = k  > 1e-3 && k  < 1e3
                   && wB > 1e-3 && wB < 1e3
                   && wC > 1e-3 && wC < 1e3;

            // Report the computed fit even when it was rejected by the guard
            // above — seeing *why* (which of k/wB/wC is out of range) is the
            // point of exposing these.
            dbgPApex = P_apex;
            dbgMu = mu; dbgNu = nu; dbgRho = rho;
            dbgK  = k;  dbgWB = wB; dbgWC  = wC;
        }
        dbgUsedApex = useApex;

        if (!useApex) {
            // Original single-corner (d only) affine construction.
            mat4 uMat = mat4(texPos[0],    texPos[1],    texPos[2],    texPos[3]);
            mat4 pMat = mat4(verteces[0],  verteces[1],  verteces[2],  verteces[3]);
            mat4 vMat = mat4(uprismPos[0], uprismPos[1], uprismPos[2], uprismPos[3]);
            mat4 invP = inverse(pMat);
            M = uMat * invP;
            T = vMat * invP;
        } else {
            // N: global space -> apex-centred coordinates (affine)
            mat4 N = mat4(
                vec4(Linv[0], 0.0),
                vec4(Linv[1], 0.0),
                vec4(Linv[2], 0.0),
                vec4(-Linv * P_apex, 1.0)
            );

            // R: apex-coords -> unit prism space. The top face stays exactly
            // at y=1 (no tilt needed — see the plan doc's matrix derivation),
            // so intersectUnitPrism() needs no change.
            mat4 R = mat4(
                vec4(0.0, k,  0.0, 1.0),
                vec4(wB,  k,  0.0, wB),
                vec4(0.0, k,  wC,  wC),
                vec4(0.0, -k, 0.0, 0.0)
            );

            // R_M: apex-coords -> texture space (shares R's W row, since the
            // height convention — h=1 at d,e,f — is the same for both targets).
            vec2 ua = vs_out_tex[0], ub = vs_out_tex[1], uc = vs_out_tex[2];
            mat4 R_M = mat4(
                vec4(ua.x,      ua.y,      k,  1.0),
                vec4(wB * ub.x, wB * ub.y, k,  wB),
                vec4(wC * uc.x, wC * uc.y, k,  wC),
                vec4(0.0,       0.0,       -k, 0.0)
            );

            M = R_M * N;
            T = R   * N;
        }
    }

    // -- Debug intersection (once per primitive) -------------------------------
    // Reads config from debugVisualSSBO[5-6] (CPU-written before this draw call).
    // Writes step positions to debugVisualSSBO[9+] and numerical data to
    // debugNumericalSSBO.debugNumerical[17..27] (per-step data starts at [28]).
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
            vec3 UnitEye = perspectiveDiv(T * vec4(dbgEye, 1.0));
            vec3 UnitAt  = perspectiveDiv(T * vec4(dbgAt,  1.0));
            vec3 UnitDir = normalize(UnitAt - UnitEye);

            UnitIntersection uiInt = intersectUnitPrism(Ray(UnitEye, UnitDir));
            if (uiInt.found) {
                mat4 M_invT    = M * inverse(T);
                vec3 TexEye    = perspectiveDiv(M * vec4(dbgEye, 1.0));

                vec3 UnitEnter = UnitEye + uiInt.near * UnitDir;
                vec3 UnitExit  = UnitEye + uiInt.far  * UnitDir;
                bool cameraInside = uiInt.near < 0.0 && uiInt.far > 0.0;
                vec3 TexEnter  = cameraInside ? TexEye : perspectiveDiv(M_invT * vec4(UnitEnter, 1.0));
                vec3 TexExit   = perspectiveDiv(M_invT * vec4(UnitExit, 1.0));

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
                debugNumerical[12] = vec4(UnitEye, 0.0);
                debugNumerical[13] = vec4(TexEnter, 0.0);
                debugNumerical[14] = vec4(TexExit,  0.0);
                debugNumerical[15] = vec4(normalize(TexExit - TexEnter), 0.0);
                debugNumerical[16] = result.wasHit ? vec4(result.uvh, 1.0) : vec4(0.0);

                // Primitive vertices: scene-space positions and UV coordinates
                // slots [17..19] = vertex[0..2] scene xyz,  slots [20..22] = vertex[0..2] UV
                debugNumerical[17] = vec4(verteces[0].xyz, 0.0);
                debugNumerical[18] = vec4(verteces[1].xyz, 0.0);
                debugNumerical[19] = vec4(verteces[2].xyz, 0.0);
                debugNumerical[20] = vec4(texPos[0].xy, 0.0, 0.0);
                debugNumerical[21] = vec4(texPos[1].xy, 0.0, 0.0);
                debugNumerical[22] = vec4(texPos[2].xy, 0.0, 0.0);

                // Apex-fit diagnostics (each vec3 + 1 scalar packed into one vec4):
                // slot 23 = d, mu       slot 24 = e, nu        slot 25 = f, rho
                // slot 26 = P_apex, usedApex (1.0/0.0)
                // slot 27 = (k, wB, wC, unused)
                // All zero (and usedApex = 0.0) when the normals were already too
                // close to parallel for S to be invertible (apex never computed).
                debugNumerical[23] = vec4(dbgD, dbgMu);
                debugNumerical[24] = vec4(dbgE, dbgNu);
                debugNumerical[25] = vec4(dbgF, dbgRho);
                debugNumerical[26] = vec4(dbgPApex, dbgUsedApex ? 1.0 : 0.0);
                debugNumerical[27] = vec4(dbgK, dbgWB, dbgWC, 0.0);
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
    gs_out_TexEye     = perspectiveDiv(M * vec4(cameraData.eye, 1.0));
    gs_out_Scene2Unit = T;
    gs_out_UnitEye    = perspectiveDiv(T * vec4(cameraData.eye, 1.0));

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
