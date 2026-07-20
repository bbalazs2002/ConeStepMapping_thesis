#version 430

in vec3 gs_out_tex;
in vec3 gs_out_norm;
in vec3 gs_out_merged;
in vec3 gs_out_pos;
in mat4 gs_out_Scene2Tex;
in vec3 gs_out_TexEye;
in mat4 gs_out_Scene2Unit;
in vec3 gs_out_UnitEye;
in mat3x2 gs_out_triangle;

out vec4 fs_out_col;

// Ray march parameters (set by LinearSearch::SetUniforms)
uniform int   maxSteps            = 64;
uniform bool  discardFragments    = true;
uniform bool  displayNonConverged = false;
uniform vec3  lightDir            = vec3(0.0, -1.0, 0.0);

layout(binding = 4) uniform sampler2D coneMap;

#include "../Modules/Camera/Camera_uniforms.glsl"
#include "RayMarch_common.glsl"

void main()
{
    vec3 p  = gs_out_pos;
    mat4 T  = gs_out_Scene2Unit;
    mat4 M  = gs_out_Scene2Tex;

    vec3 p0 = gs_out_UnitEye;
    vec3 p1 = (T * vec4(p, 1.0)).xyz;
    vec3 v  = normalize(p1 - p0);

    UnitIntersection unitInt = intersectUnitPrism(Ray(gs_out_UnitEye, v));
    if (!unitInt.found) {
        if (discardFragments) discard;
        fs_out_col = vec4(0.0, 1.0, 1.0, 1.0);
        return;
    }

    // Entry and exit in texture space.
    // When near < 0 the camera is inside the prism; start from the camera position
    // in texture space instead of the rasterised face's texcoord.
    vec3 uu = gs_out_UnitEye + unitInt.far * v;
    vec3 u1 = (unitInt.near < 0.0) ? gs_out_TexEye : gs_out_tex;
    vec3 u2 = (M * inverse(T) * vec4(uu, 1.0)).xyz;

    IntersectReturn data = findIntersection_linearSearch(
        IntersectParams(u1, u2, gs_out_TexEye));

    if (!data.wasHit) {
        if (displayNonConverged) {
            fs_out_col = vec4(1.0, 0.0, 1.0, 1.0);
        } else if (discardFragments) {
            discard;
        } else {
            fs_out_col = vec4(0.0, 1.0, 0.0, 1.0);
        }
        return;
    }

    vec2 uv = data.uv;

    vec3 ambient = vec3(conemap_getHeight(uv)) * 0.5;
    vec3 norm    = ls_sampleNormal(uv);
    vec3 viewDir = normalize(cameraData.eye - gs_out_pos);

    float diffuse  = 0.4 * max(dot(normalize(lightDir.xzy), norm), 0.0);
    vec3  reflDir  = reflect(-normalize(lightDir.xzy), norm);
    float specular = 0.3 * pow(max(dot(viewDir, reflDir), 0.0), 32.0);

    fs_out_col = vec4(ambient + diffuse + specular, 1.0);
}
