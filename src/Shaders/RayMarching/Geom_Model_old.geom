#version 430 core

#define GEOMETRY_SHADER

in vec2 vs_out_tex[];
in vec3 vs_out_norm[];
in vec3 vs_out_merged[];            // merged normals

out vec3 gs_out_tex;
out vec3 gs_out_norm;
out vec3 gs_out_merged;
out vec3 gs_out_pos;
out mat4 gs_out_Scene2Tex;          // transformation from scene space to texture space
out vec3 gs_out_TexEye;             // cam position in texture space
out mat4 gs_out_Scene2Unit;         // transformation from scene space to unit prism space
out vec3 gs_out_UnitEye;            // cam position in unit prism space
out mat3x2 gs_out_triangle;         // base triangle verteces in texture space

uniform mat4 viewProj;
uniform float modelNormalMult = .5;
uniform vec3 camPos;
uniform vec2 HMres;

uniform int rayMarchingTechnique = 0;
uniform int showSteps = 0;
uniform int showEnterExit = 0;

layout(triangles) in;  
layout(triangle_strip, max_vertices = 18) out;

#define SSBOPADDING 7

//
// Include common file with helper functions
//
#include "Glsl_common.glsl"

void main() {

    // p - scene space positions
    vec4 verteces[] = {
        gl_in[0].gl_Position,                                                     // a - 0
        gl_in[1].gl_Position,                                                     // b - 1
        gl_in[2].gl_Position,                                                     // c - 2
        (gl_in[0].gl_Position + vec4(vs_out_merged[0] * modelNormalMult, 0.0)),   // d - 3
        (gl_in[1].gl_Position + vec4(vs_out_merged[1] * modelNormalMult, 0.0)),   // e - 4
        (gl_in[2].gl_Position + vec4(vs_out_merged[2] * modelNormalMult, 0.0))    // f - 5
    };

    // u - texture space positions
    vec4 texPos[] = {
        vec4(vs_out_tex[0], 0, 1),
        vec4(vs_out_tex[1], 0, 1),
        vec4(vs_out_tex[2], 0, 1),
        vec4(vs_out_tex[0], 1, 1),
        vec4(vs_out_tex[1], 1, 1),
        vec4(vs_out_tex[2], 1, 1)
    };

    // v - unit prism space positions
    vec4 uprismPos[] = {
        vec4(0, 0, 0, 1),
        vec4(1, 0, 0, 1),
        vec4(0, 0, 1, 1),
        vec4(0, 1, 0, 1)
    };

    gs_out_triangle = mat3x2(
        texPos[0].xy,
        texPos[1].xy,
        texPos[2].xy
    );

    // M = [u0 u1 u2 u3] * [p0 p1 p2 p3]^-1
    mat4 u = {
        texPos[0],
        texPos[1],
        texPos[2],
        texPos[3]
    };
    mat4 p = {
        verteces[0],
        verteces[1],
        verteces[2],
        verteces[3]
    };
    mat4 v = {
        uprismPos[0],
        uprismPos[1],
        uprismPos[2],
        uprismPos[3]
    };

    mat4 invP = inverse(p);
    mat4 M = u * invP;          // transformation from scene to texture
    mat4 T = v * invP;          // transformation from scene to unit prism

    mat4 invM = inverse(M);
    mat4 invT = inverse(T);

    // setup pipeline variables
    gs_out_Scene2Tex = M;
    gs_out_TexEye = (M * vec4(camPos, 1)).xyz;
    gs_out_Scene2Unit = T;
    gs_out_UnitEye = (T * vec4(camPos, 1)).xyz;

    // set numerical debug values
    numericalDebugSet(3, M);
    numericalDebugSet(7, M * numericalDebugGet(1));
    numericalDebugSet(8, T);
    numericalDebugSet(12, T * numericalDebugGet(1));

    // save texture to world transformation to the SSBO
    visualDebugSet(1, invM);

    // find entry and exit points
    vec4 p0 = T * visualDebugGet(5);
    vec4 p1 = T * visualDebugGet(6);
    vec3 direction = normalize((p1 - p0).xyz);
    float tNear = 0;
    float tFar = 0;

    UnitIntersection unitInt = intersectUnitPrism(Ray(p0.xyz, direction));
    visualDebugSet(0, vec4(0));
    if (unitInt.found) {

        // transform enter and exit points to texture space
        vec4 pNear = M * invT * (p0 +  vec4(direction * unitInt.near, 0));
        vec4 pFar = M * invT * (p0 + vec4(direction * unitInt.far, 0));

        numericalDebugSet(13, pNear);
        numericalDebugSet(14, pFar);

        int pointCount = 0;
        int vDebugIndex = SSBOPADDING;
        if (showEnterExit > 0) {
            visualDebugSet(vDebugIndex++, pNear);
        }


        IntersectReturn data;
        if (rayMarchingTechnique == 0) {
            data = findIntersection_linearSearch(IntersectParams(pNear.xyz, pFar.xyz, (M * numericalDebugGet(1)).xyz, vDebugIndex));
        } else {    // 1
            data = findIntersection_coneStepMapping(IntersectParams(pNear.xyz, pFar.xyz, (M * numericalDebugGet(1)).xyz, vDebugIndex));
        }
        
        vDebugIndex += data.stepCount;
        if (showEnterExit > 0) {
            visualDebugSet(vDebugIndex++, pFar);
        }

        visualDebugSet(0, vec4(vDebugIndex - 5));
        numericalDebugSet(0, vec4(data.stepCount, data.flags, 0, 0));
    }

    // draw unit prism
    /*
    for (int i = 0; i < 6; ++i) {
        pos[SSBOPadding + (pcount++)] = T * verteces[i]; // uprismPos[i];
    }
    */

    //
    // TRIANGLE STRIP
    //

    /*
    // e d b a c d f e c b
    // 4 3 1 0 2 3 5 4 2 1
    int indeces[] = {4, 3, 1, 0, 2, 3, 5, 4, 2, 1};

    for (int i = 0; i < 10; ++i) {
        gs_out_tex = texPos[indeces[i]];
        gs_out_norm = vs_out_norm[int(mod(indeces[i], 3))];
        gs_out_pos = verteces[indeces[i]];
        gl_Position = verteces[indeces[i]];
        EmitVertex();
    }
    EndPrimitive();
    */

    //
    // TRIALNGLES
    //

    // Bottom
    gs_out_tex = texPos[0].xyz;
    gs_out_norm = -vs_out_norm[0];
    gs_out_merged = -vs_out_merged[0];
    gs_out_pos = verteces[0].xyz;
    gl_Position = viewProj * verteces[0];
    EmitVertex();

    gs_out_tex = texPos[1].xyz;
    gs_out_norm = -vs_out_norm[1];
    gs_out_merged = -vs_out_merged[1];
    gs_out_pos = verteces[1].xyz;
    gl_Position = viewProj * verteces[1];
    EmitVertex();
    gs_out_tex = texPos[2].xyz;
    gs_out_norm = -vs_out_norm[2];
    gs_out_merged = -vs_out_merged[2];
    gs_out_pos = verteces[2].xyz;
    gl_Position = viewProj * verteces[2];
    EmitVertex();
    EndPrimitive();

    // Top
    gs_out_tex = texPos[3].xyz;
    gs_out_norm = vs_out_norm[0];
    gs_out_merged = vs_out_merged[0];
    gs_out_pos = verteces[3].xyz;
    gl_Position = viewProj * verteces[3];
    EmitVertex();
    gs_out_tex = texPos[4].xyz;
    gs_out_norm = vs_out_norm[1];
    gs_out_merged = vs_out_merged[1];
    gs_out_pos = verteces[4].xyz;
    gl_Position = viewProj * verteces[4];
    EmitVertex();
    gs_out_tex = texPos[5].xyz;
    gs_out_norm = vs_out_norm[2];
    gs_out_merged = vs_out_merged[2];
    gs_out_pos = verteces[5].xyz;
    gl_Position = viewProj * verteces[5];
    EmitVertex();
    EndPrimitive();

    // Side 1
    vec3 norm1 = normalize(cross(verteces[0].xyz - verteces[1].xyz, verteces[0].xyz - verteces[3].xyz));
    gs_out_norm = norm1;
    gs_out_merged = norm1;

    gs_out_tex = texPos[0].xyz;
    gs_out_pos = verteces[0].xyz;
    gl_Position = viewProj * verteces[0];
    EmitVertex();
    gs_out_tex = texPos[1].xyz;
    gs_out_pos = verteces[1].xyz;
    gl_Position = viewProj * verteces[1];
    EmitVertex();
    gs_out_tex = texPos[3].xyz;
    gs_out_pos = verteces[3].xyz;
    gl_Position = viewProj * verteces[3];
    EmitVertex();
    gs_out_tex = texPos[4].xyz;
    gs_out_pos = verteces[4].xyz;
    gl_Position = viewProj * verteces[4];
    EmitVertex();
    EndPrimitive();

    // Side 2
    vec3 norm2 = normalize(cross(verteces[1].xyz - verteces[4].xyz, verteces[2].xyz - verteces[1].xyz));
    gs_out_norm = norm2;
    gs_out_merged = norm2;

    gs_out_tex = texPos[1].xyz;
    gs_out_pos = verteces[1].xyz;
    gl_Position = viewProj * verteces[1];
    EmitVertex();
    gs_out_tex = texPos[2].xyz;
    gs_out_pos = verteces[2].xyz;
    gl_Position = viewProj * verteces[2];
    EmitVertex();
    gs_out_tex = texPos[4].xyz;
    gs_out_pos = verteces[4].xyz;
    gl_Position = viewProj * verteces[4];
    EmitVertex();
    gs_out_tex = texPos[5].xyz;
    gs_out_pos = verteces[5].xyz;
    gl_Position = viewProj * verteces[5];
    EmitVertex();
    EndPrimitive();

    // Side 3
    vec3 norm3 = normalize(cross(verteces[0].xyz - verteces[2].xyz, verteces[5].xyz - verteces[2].xyz));
    gs_out_norm = norm3;
    gs_out_merged = norm3;

    gs_out_tex = texPos[2].xyz;
    gs_out_pos = verteces[2].xyz;
    gl_Position = viewProj * verteces[2];
    EmitVertex();
    gs_out_tex = texPos[0].xyz;
    gs_out_pos = verteces[0].xyz;
    gl_Position = viewProj * verteces[0];
    EmitVertex();
    gs_out_tex = texPos[5].xyz;
    gs_out_pos = verteces[5].xyz;
    gl_Position = viewProj * verteces[5];
    EmitVertex();
    gs_out_tex = texPos[3].xyz;
    gs_out_pos = verteces[3].xyz;
    gl_Position = viewProj * verteces[3];
    EmitVertex();
    EndPrimitive();
}