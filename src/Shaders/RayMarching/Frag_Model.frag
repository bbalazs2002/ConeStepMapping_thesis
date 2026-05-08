#version 430

//
// VARIABLES IN THE PIPELINE
//
in vec3 gs_out_tex;
in vec3 gs_out_norm;
in vec3 gs_out_merged;
in vec3 gs_out_pos;
in mat4 gs_out_M;
in vec3 gs_out_Meye;
in mat4 gs_out_T;
in vec3 gs_out_Teye;
in mat3x2 gs_out_triangle;

out vec4 fs_out_col;

//
// UNIFORMS
//

// textures
uniform sampler2D texImage;
uniform sampler2D coneMap;

// uniforms
uniform vec2 HMres;     // height map resolution (w, h)
uniform vec2 HMres_r;   // reciprical of the height map resolution (1/w, 1/h)
uniform float relax = 1.;
uniform int maxSteps = 50;
uniform vec3 camPos;
uniform bool discardFragments = true;
uniform vec3 lightDir = vec3(0,-1.,0);
uniform float lightIntensity = 1.;
uniform bool displayNonConverged = false;
uniform float epsilon = 0.0;

uniform int refine_steps = 1;

uniform int rayMarchingTechnique = 0;
uniform int showFlags = 0;

//
// INTERSECTION DATA
//
struct HMapIntersection{
    vec2 uv;
    float t;
    float last_t;
    bool wasHit;
};
const HMapIntersection INIT_INTERSECTION = { 0.0.xx, 0.0, 0.0, false };

//
// HELPER FUNCTIONS
//
vec2 getHC_texture(vec2 uv) {
    return texture(coneMap, uv);    // .r is the height; .g is the tangent of the cone
}
float getH(vec2 uv) {
    return getHC_texture(uv).r;
}
// unit prism intersection
bool intersectUnitPrism(vec3 p0, vec3 v, out float tNear, out float tFar) {     // p0, v in unit prism space
    float n = -1e10, f = 1e10; // near, far

    vec3 t0 = -p0 / v; // a solution for each cardinal normal
    if (v.x > 0.) { n = max(n, t0.x); } else { f = min(f, t0.x); }
    if (v.y > 0.) { n = max(n, t0.y); } else { f = min(f, t0.y); }
    if (v.z > 0.) { n = max(n, t0.z); } else { f = min(f, t0.z); }

    vec3 q = vec3(1, 1, 0) - p0;
    float t1 = q.y / v.y;
    if (v.y < 0.) { n = max(n, t1); } else { f = min(f, t1); }
    float t2 = (q.x + q.z) / (v.x + v.z);
    if (v.x + v.z < 0.) { n = max(n, t2); } else { f = min(f, t2); }

    tNear = n;
    tFar = f;
    return n < f;
}

vec3 getNormalTBN_finiteDiff(vec2 uv) {
    const float mutliplier = 1.;
    vec2 delta = HMres_r * mutliplier;
    vec2 one_over_delta = HMres / mutliplier;
    
    vec2 du = vec2(delta.x, 0);
    vec2 dv = vec2(0, delta.y);
    float dhdu = 0.5 * one_over_delta.x * (getH(uv + du) - getH(uv - du));
    float dhdv = 0.5 * one_over_delta.y * (getH(uv + dv) - getH(uv - dv));
    return normalize(vec3(-dhdu, -dhdv, 1));
}
vec3 getNormalTBN(vec2 uv) {
    return getNormalTBN_finiteDiff(uv);
}

//
// REFINE FUNCTIONS
//
vec2 refineIntersection_linearApprox(HMapIntersection interval, vec2 u0, vec2 u1) {
    float t0 = interval.last_t;
    float t1 = interval.t;
    float h0 = getH(mix(u0, u1, t0));
    float h1 = getH(mix(u0, u1, t1));
    float dt = t1 - t0;
    float t = (dt + t0 * h1 - t1 * h0) / (dt + h1 - h0);
    t = clamp(t, t0, t1);
    return mix(u0, u1, t);
}
vec2 refineIntersection_binarySearch(HMapIntersection interval, vec2 u0, vec2 u1) {
    float t0 = interval.last_t;
    float t1 = interval.t;
    float th = 0.5 * (t0 + t1);
    for (uint i = 0; i < refine_steps; ++i)
    {
        float fh = getH(mix(u0, u1, th));
        if (fh > 1 - th)
            t1 = th;
        else
            t0 = th;
        th = 0.5 * (t0 + t1);
    }
    
    return mix(u0, u1, th);
}

//
// RAY MARCHING FUNCTIONS
//
HMapIntersection findIntersection_linearSearch(vec3 u1, vec3 u2) {
    HMapIntersection ret = INIT_INTERSECTION;
    vec3 v = u2 - u1;

    int stepCount = 0;
    for (float t = 0.; t <= 1.1 && stepCount <= maxSteps; t += 1./64.) {
        ret.t = t;
        vec3 u = u1 + t * v;

        vec2 txt = getHC_texture(u.xy);
        if (txt.r > u.z) {
            // hit found
            return HMapIntersection(
                u.xy, min(1., t), t, true
            );
        }
        ++stepCount;
    }

    return ret;
}

float getNextT(vec3 ui, vec3 v, float tgb) {
    vec2 tex = getHC_texture(ui.xy);        // texture at the initial point
    vec3 ai = vec3(ui.xy, tex.x);           // vertex of the cone
    float tga = tex.g;

    float hi = (tgb * (ui.z - ai.z)) / (tgb + tga);
    float xi = ai.z + hi;
    float ti = (xi - ui.z) / v.z;

    return ti;
}

float getNextStep(vec3 ui, vec3 v, float tgb) { // view vector, last intersection, point on surfice under U, tangent at A
    vec2 tex = getHC_texture(ui.xy);        // texture at the initial point
    vec3 ai = vec3(ui.xy, tex.r);           // vertex of the cone

    float tga = 1. /  abs(tex.g);

    if (v.x * v.y < 0) {
        tga = -tga;
    }

    float x = ((ai.x * tga) - (ui.x / tgb) + ui.z - ai.z) / (tga - (1 / tgb));
    float y = (1. / tgb) * (x - ui.x) + ui.z;

    float t = (y - ui.z) / v.z;
    return t;
}

HMapIntersection findIntersection_coneStepMapping_new(vec3 u1, vec3 u2, out int flags) {   // u1: enter point, u2: exit point in texture space

    // pre-check
    if (getH(u1.xy) >= u1.z) {
        return HMapIntersection( vec2(u1.xy), 0, 0, true );
    }

    vec3 v = normalize(u2 - u1);            // direction vector from u1 to u2

    float tgb = length(v.xy) / v.z;      // tangent between v and -normal
    if (v.y < 0) {
        tgb = -tgb;
    }

    float maxT = ((u2 - u1) / v).x;      // t parameter of the exit point (u2)
    vec3 ui = u1 + 0.0001 * v;

    float t = 0.0;                       // t parameter of the intersection point (u1 -> ui)
    float ti = t + 1.;                   // t parameter of the current step (ui -> ui+1)

    int stepCount = 0;
    
    while(
        stepCount <= maxSteps &&        // max step count reached => divergent
        t < maxT + .001 &&       	    // Stay within prism
        ti > 0.0001 * t                 // Stop if cone is close to surface
    ) {
        ti = getNextStep(u1 + t * v, v, tgb);

        t = min(maxT, t + ti);

        // t += ti;
        ++stepCount;
    }

    /*
    do {
        ti = getNextT(u1 + t * v, v, tgb);
        t += ti;
        ++stepCount;
    } while (
        stepCount <= maxSteps &&        // max step count reached => divergent
        t < maxT + .001 && 	            // Stay within prism
        ti > 0.0001 * t                 // Stop if cone is close to surface
    );
    */

    flags = int(stepCount > maxSteps) |
            (int(t >= maxT + .001) << 1) |
            (int(ti <= 0.0001 * t)  << 2);

    /*
    if (t >= maxT + .001) {
        discard;
    }
    */

    HMapIntersection val = INIT_INTERSECTION;

    val.wasHit = bool(flags & 4) && !bool(flags ^ 4);

    val.t = t;
    val.uv = (u1 + v * t).xy;

    return val;
}

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
    mat4 T = gs_out_T;
    mat4 M = gs_out_M;

    // vec3 Meye = gs_out_Meye;        // camera position in tangent space
    // vec3 v = normalize(u1 - Meye);  // direction of ray in tangent space

    vec3 p0 = gs_out_Teye;
    vec3 p1 = (T * vec4(p, 1)).xyz;
    vec3 v = normalize(p1 - p0);

    float tNear = 0;
    float tFar = 0;
    if (!intersectUnitPrism(gs_out_Teye, v, tNear, tFar)) {     // cannot happen
        if (discardFragments) {
            discard;
        }
        fs_out_col = vec4(0, 1, 1, 1);
        return;
    }

    vec3 uu = gs_out_Teye + tFar * v;               // exit point in unit prism space
    vec3 u2 = (M * inverse(T) * vec4(uu, 1)).xyz;   // exit point in texture space
    
    // find the intersection with the height map
    HMapIntersection I = INIT_INTERSECTION;
    if (rayMarchingTechnique == 0) {
        I = findIntersection_linearSearch(u1, u2);
    } else if (rayMarchingTechnique == 1) {
        int flags;
        I = findIntersection_coneStepMapping_new(u1, u2, flags);

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

            if (I.wasHit) {
                flagCol *= 2.;
            }

            fs_out_col = vec4(flagCol, 1);
            return;
        }
        
        if (!I.wasHit) {
            discard;
        }

        // fs_out_col = vec4(int(I.wasHit));
        // fs_out_col = vec4(I.uv, 0, 1.);
        // fs_out_col = vec4(I.t, 0, 0, 1.);
        // return;

    }
    // HMapIntersection I = findIntersection_bumpMapping(u, u2);
    // HMapIntersection I = findIntersection_linearSearch(u1.xyz, u2.xyz);
    // HMapIntersection I = findIntersection_coneStepMapping(u1.xy, u2.xy);
    // HMapIntersection I =  findIntersection_coneStepMapping_new(u1, u2);

    // vec2 u3 = refineIntersection_linearApprox(I, u, u2);
    vec2 u3 = I.uv; // no refine function applied
    
    /*
    // fetch the final albedo color
    vec3 albedo = texture(texImage, u3).xyz;
    col *= albedo;
    
    vec3 norm = vec3(0,1,0);

    float diffuse = lightIntensity * clamp(dot(-normalize(lightDir), norm), 0, 1.);
    col.rgb *= diffuse;
    */
    col = texture(coneMap, u3).xyz;

    // return;
    if (!I.wasHit) {
        if (displayNonConverged) {
            col = vec3(1,0,1);
        } else if (discardFragments) {
            discard;
        } else {
            col = vec3(0, 1, 0);
        }
    }
    
    fs_out_col = vec4(col, 1.);
}