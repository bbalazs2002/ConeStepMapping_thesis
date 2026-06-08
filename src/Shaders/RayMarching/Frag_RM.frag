#version 430

//
// VARIABLES IN THE PIPELINE
//
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

//
// UNIFORMS
//

// uniforms
uniform vec2 HMres;     // height map resolution (w, h)
uniform vec2 HMres_r;   // reciprical of the height map resolution (1/w, 1/h)
uniform vec3 camPos;
uniform bool discardFragments = true;
uniform vec3 lightDir = vec3(0,-1.,0);
uniform bool displayNonConverged = false;

uniform int rayMarchingTechnique = 0;       // 0 - linear; 1 - cone-step mapping
uniform int showFlags = 0;

//
// Include common file with helper functions
//
#include "Glsl_common.glsl"
#include "../Modules/Material/Material_uniforms.glsl"
#include "../Modules/Material/Material.glsl"

//
// MAIN FUNCTION
//
void main() {

    // fs_out_col = vec4(gs_out_norm * .5 + .5, 1);
    // fs_out_col = vec4(1,0,0,1);
    // fs_out_col = vec4(abs(gs_out_norm), 1);
    // return;

    vec3 col = vec3(.5);

    vec3 u1 = gs_out_tex;           // original texcoords
    vec3 p = gs_out_pos;            // fragment world pos
    mat4 T = gs_out_Scene2Unit;
    mat4 M = gs_out_Scene2Tex;

    vec3 p0 = gs_out_UnitEye;
    vec3 p1 = (T * vec4(p, 1)).xyz;

    vec3 v = normalize(p1 - p0);

    UnitIntersection unitInt = intersectUnitPrism(Ray(gs_out_UnitEye, v));
    if (!unitInt.found) {           // cannot happen
        if (discardFragments) {
            discard;
        }
        fs_out_col = vec4(0, 1, 1, 1);
        return;
    }

    vec3 uu = gs_out_UnitEye + unitInt.far * v;         // exit point in unit prism space
    vec3 u2 = (M * inverse(T) * vec4(uu, 1)).xyz;       // exit point in texture space
    
    // find the intersection with the height map
    IntersectReturn data;
    if (rayMarchingTechnique == 0) {
        data = findIntersection_linearSearch(IntersectParams(
            u1, u2,             // enter, exit
            gs_out_TexEye,      // camera position
            0                   // vDebugStart
        ));
    } else {    // 1
        data = findIntersection_coneStepMapping(IntersectParams(
            u1, u2,             // enter, exit
            gs_out_TexEye,      // camera position
            0                   // vDebugStart
        ));
    }
    int flags = data.flags;

    if (showFlags > 0) {

        vec3 flagCol = vec3(0, 0, 0);
        if (bool(flags & 1)) {
            flagCol.r = .5;
        }
        if (bool(flags & 2)) {
            flagCol.g = .5;
        }
        if (bool(flags & 4)) {
            flagCol.b = .5;
        }

        if (data.wasHit) {
            flagCol *= 2.;
        }

        fs_out_col = vec4(flagCol, 1);
        return;
    }
        
    // fs_out_col = vec4(0, 0, I.wasHit, 1);
    // return;

    // return if not hit
    if (!data.wasHit) {
        if (displayNonConverged) {
            fs_out_col = vec4(1,0,1, 1);
        } else if (discardFragments) {
            discard;
        } else {
            fs_out_col = vec4(0, 1, 0, 1);
        }
        return;
    }

    // fs_out_col = vec4(int(I.wasHit));
    // fs_out_col = vec4(I.uv, 0, 1.);
    // fs_out_col = vec4(I.t, 0, 0, 1.);
    // return;

    // vec2 u3 = refineIntersection_linearApprox(I, u, u2);
    vec2 u3 = data.uv; // no refine function applied
    
    // fetch the final albedo color
    // vec3 albedo = texture(texImage, u3).xyz;
    // col *= albedo;

    float[13] material = MaterialPrepare(u3);

    vec3 ambient = vec3(material[3], material[4], material[5]) * .5f;
    vec3 norm = getNormalFromHeightmap(u3);
    float DiffuseFactor = .4f * max(dot(normalize(lightDir.xzy), norm), 0.0) * 1.f;
    float diffuse = DiffuseFactor;


	vec3 viewDir = normalize( camPos - gs_out_pos );
	vec3 reflectDir = reflect( -normalize(lightDir.xzy), norm );
    float SpecularFactor = pow(max( dot( viewDir, reflectDir) ,0.0), 32.) * .3;
	float specular = SpecularFactor;

    col.rgb = ambient + diffuse + specular + vec3(data.uv, 0) * .5;

    //col = texture(coneMap, u3).xyz;

    // col = getNormalFromHeightmap(u3) * .5 + .5;

    fs_out_col = vec4(col, 1.);
}