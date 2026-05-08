// debug buffers
layout(std430, binding = 0) buffer VisDebug{
    vec4 vdbug[];                                 // buffer for visual debug
};

layout(std430, binding = 1) buffer NumDebug{
    vec4 ndbug[];                                // buffer for numerical debug
};

// uniforms
uniform sampler2D coneMap;
uniform int maxSteps = 50;

// structs
struct UnitIntersection{
    bool found;
    float near;
    float far;
};
struct Ray{
    vec3 start;
    vec3 dir;
};
struct IntersectParams{
    vec3 enter;
    vec3 exit;
    vec3 cam;
    int vDebugStart;
};
struct IntersectReturn{
    vec2 uv;
    float t;
    float last_t;
    bool wasHit;

    int flags;
    int stepCount;
};
struct StepParams {
    vec3 point;
    Ray view;
    vec2 tex;
};
struct StepReturn{
    float t;
};

#ifdef GEOMETRY_SHADER
// debug functionality for the geometry shader
void visualDebugSet(int index, vec4 data) {
    vdbug[index] = data;
}
void visualDebugSet(int index, mat4x4 data) {
    for (int i = 0; i < 4; ++i) {
        vdbug[index + i] = data[i];
    }
}
vec4 visualDebugGet(int index) {
    return vdbug[index];
}
void numericalDebugSet(int index, vec4 data) {
    ndbug[index] = data;
}
void numericalDebugSet(int index, mat4x4 data) {
    for (int i = 0; i < 4; ++i) {
        ndbug[index + i] = data[i];
    }
}
vec4 numericalDebugGet(int index) {
    return ndbug[index];
}
#else
// debug is not needed in the fragment shader
void visualDebugSet(int index, vec4 data) {}
void visualDebugSet(int index, mat4x4 data) {}
vec4 visualDebugGet(int index) {return vec4(0);}
void numericalDebugSet(int index, vec4 data) {}
void numericalDebugSet(int index, mat4x4 data) {}
vec4 numericalDebugGet(int index) {return vec4(0);}
#endif

// conemap texture handling
vec2 conemap_get(vec2 uv) {
    return texture(coneMap, uv);    // .r is the height; .g is the tangent of the cone
}
float conemap_getHeight(vec2 uv) {
    return conemap_get(uv).r;
}

vec3 getNormalFromHeightmap(vec2 uv) {
    float eps = 0.001;
    float h = texture(coneMap, uv).r;
    float hx = texture(coneMap, uv + vec2(eps, 0.0)).r;
    float hy = texture(coneMap, uv + vec2(0.0, eps)).r;
    
    vec3 dx = vec3(eps, 0.0, hx - h);
    vec3 dy = vec3(0.0, eps, hy - h);
    return normalize(cross(dx, dy));
}

StepReturn getNextStep_old_260424(StepParams params) {       // current intersection point, view vector (u1 -> u2)

    vec3 u = params.point;
    vec3 e = params.view.start;
    vec3 v = params.view.dir;

    StepReturn val;

    // old intersection
    /*
    float az = params.tex.r;           // vertex of the cone
    float ctga = 1.f / params.tex.g;
    float sq = ((e.y - u.y) / (e.x - u.x)) * ((e.y - u.y) / (e.x - u.x));
    float gamma = sqrt(1.f + sq);
    float t1 = (az - u.z) / (v.z - gamma * v.x * ctga);
    float t2 = (az - u.z) / (v.z + gamma * v.x * ctga);
    */

    // new intersection
    float az = params.tex.r;            // height
    float ctga = 1.f / params.tex.g;    // 1 / tg
    // float m = abs(sqrt((e.x - u.x) * (e.x - u.x) + (e.y - u.y) * (e.y - u.y))) * ctga;
    float m = length(u.xy - e.xy) * ctga;
    float t1 = 0.f;
    float t2 = 0.f;
    if (abs(e.x - u.x) > 0.0001f)  {        // e.x - u.x > 0
        t1 = -(v.x * (az - u.z)) / (v.z * (e.x - u.x) + m * v.x);
        t2 = -(v.x * (az - u.z)) / (v.z * (e.x - u.x) - m * v.x);
    } else {                                // e.y - u.y > 0
        t1 = -(v.y * (az - u.z)) / (v.z * (e.y - u.y) + m * v.y);
        t2 = -(v.y * (az - u.z)) / (v.z * (e.y - u.y) - m * v.y);
    }
    
    val.t = max(t1, t2);
    return val;
}

StepReturn getNextStep(StepParams params) {
    vec3 u = params.point;
    vec3 v = params.view.dir;

    float az   = params.tex.r;
    float ctga = 1.0 / params.tex.g;
    float dz   = az - u.z;
    float horiz = length(v.xy);

    float denom1 = v.z + horiz * ctga;
    float denom2 = v.z - horiz * ctga;

    float t1 = abs(denom1) > 1e-7 ? dz / denom1 : -1.0;
    float t2 = abs(denom2) > 1e-7 ? dz / denom2 : -1.0;

    StepReturn val;
    val.t = max(t1, t2);
    return val;
}

IntersectReturn findIntersection_linearSearch(IntersectParams params) {
    vec3 v = params.exit - params.enter;
    int stepCount = 0;
    for (float t = 0.; t <= 1.1 && stepCount <= maxSteps; t += 1./64.) {
        vec3 u = params.enter + t * v;
        vec2 tex = conemap_get(u.xy);

        visualDebugSet(params.vDebugStart + stepCount, vec4(u, 1));
        numericalDebugSet(17 + stepCount * 2, vec4(1./64., t, 1., tex.x));
        numericalDebugSet(17 + stepCount * 2 + 1, vec4(u, 0));

        if (tex.r > u.z) {
            // hit found
            return IntersectReturn(
                u.xy,          // uv
                t, t,       // t, last_t
                true,           // wasHit
                0, stepCount            // flags, stepCount
            );
        }
        ++stepCount;
    }

    return IntersectReturn(
        vec2(0),        // uv
        0.f, 0.f,       // t, last_t
        false,          // wasHit
        0, stepCount    // flags, stepCount
    );
}

IntersectReturn findIntersection_coneStepMapping(IntersectParams params) {
    vec3 u1 = params.enter;
    vec3 u2 = params.exit;
    vec3 e = params.cam;
    int stepCount = 0;
    int flags = 0;

    // pre-check
    if (conemap_getHeight(u1.xy) >= u1.z) {

        visualDebugSet(params.vDebugStart, vec4(u1, 1.));

        flags = int(false) |
            (int(false) << 1) |
            (int(false)  << 2);

        numericalDebugSet(0, vec4(stepCount, 0, 0, 0));
        return IntersectReturn(
            u1.xy,          // uv
            0.f, 0.f,       // t, last_t
            true,           // wasHit
            flags, 0            // flags, stepCount
        );
    }

    // vec3 v = u2 - u1;        // direction vector from u1 to u2
    vec3 v = u1 - e;            // direction vector from e to u1
    // vec3 v = vec3(1,2,3);

    numericalDebugSet(15, vec4(v, 0));

    float maxT = 10000000.f;        // t parameter of the exit point (u2)

    // maxT = max((u2 - e / v).x, max((u2 - e / v).y, (u2 - e / v).y));
    if (abs(v.x) > 0.0001f) {
        maxT = (u2.x - e.x) / v.x;             // t parameter of the intersection point (e -> u2)
    } else if (abs(v.y) > 0.0001f) {
        maxT = (u2.y - e.y) / v.y;             // t parameter of the intersection point (e -> u2)
    } else {
        maxT = (u2.z - e.z) / v.z;             // t parameter of the intersection point (e -> u2)
    }
    
    float t = 1.000001f;                // t parameter of the intersection point (e -> ui)
    vec3 ui = e + t * v;
    vec2 tex = conemap_get(ui.xy);
    float last_t = 0.f;
    float ti = t + 1.f;                 // t parameter of the current step (ui -> ui+1)

    // numericalDebugSet(16, vec4(t, maxT, e.x, u1.x));
    numericalDebugSet(16, vec4(t, maxT, tex.r, tex.g));

    while(
        stepCount <= maxSteps &&            // max step count reached => divergent
        t < maxT &&       	                // Stay within prism
        ti > 1e-6                           // Stop if cone is close to surface
    ) {
        StepReturn stepData = getNextStep(StepParams(ui, Ray(e, v), tex));
        ti = stepData.t;

        if (ti < 0) {           // the ray never intersects the V shape
            return IntersectReturn(
                vec2(0, 0),
                -1, -1,
                false,
                0, 0
            );
        }

        last_t = t;
        t += ti;
        ui = e + t * v;
        tex = conemap_get(ui.xy);

        visualDebugSet(params.vDebugStart + stepCount, vec4(ui, 1));
        numericalDebugSet(17 + stepCount * 2, vec4(ti, t, maxT, tex.x));
        numericalDebugSet(17 + stepCount * 2 + 1, vec4(ui, 0));

        ++stepCount;
        
        if (ui.z <= tex.r) {    // intersection is below the surface
            ti = -1.f;
            break;
        }
        
    }

    flags = int(stepCount > maxSteps) |
            (int(t >= maxT) << 1) |
            (int(ti <= 1e-6)  << 2);

    return IntersectReturn(
        ui.xy,
        t, last_t,
        // bool(flags & 4) && !bool(flags ^ 4),
        bool(flags & 4) && !bool(flags & 1),
        flags, stepCount
    );
}

// unit prism intersection
UnitIntersection intersectUnitPrism(Ray ray) {     // ray in unit prism space
    float n = -1e10, f = 1e10; // near, far

    vec3 p0 = ray.start;
    vec3 v = ray.dir;

    vec3 t0 = -p0 / v;
    if      (v.x > 0.)       { n = max(n, t0.x); }
    else if (v.x < 0.)       { f = min(f, t0.x); }
    else if (p0.x < 0.)      { return UnitIntersection(false, 0., 0.); } // parallel, outside

    if      (v.y > 0.)       { n = max(n, t0.y); }
    else if (v.y < 0.)       { f = min(f, t0.y); }
    else if (p0.y < 0.)      { return UnitIntersection(false, 0., 0.); }

    if      (v.z > 0.)       { n = max(n, t0.z); }
    else if (v.z < 0.)       { f = min(f, t0.z); }
    else if (p0.z < 0.)      { return UnitIntersection(false, 0., 0.); }

    // top plane
    vec3 q = vec3(1, 1, 0) - p0;
    float t1 = q.y / v.y;
    if (v.y < 0.) { n = max(n, t1); } else { f = min(f, t1); }

    // diagonal plane
    float t2 = (q.x + q.z) / (v.x + v.z);
    if (v.x + v.z < 0.) { n = max(n, t2); } else { f = min(f, t2); }

    return UnitIntersection(n < f, n, f);
}