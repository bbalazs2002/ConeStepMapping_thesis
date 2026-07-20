// Shared ray marching types, helpers, and intersection algorithms.
// Included by the fragment shaders and (for visual debug) by the geometry shader.
//
// Expects the caller to declare:
//   layout(binding = 4) uniform sampler2D coneMap;
//   uniform int maxSteps;
//
// Interpolation permutations (injected via #define before #include):
//   INTERP_HEIGHT — bilinear height sampling via texture(); default: texelFetch (nearest)
//   INTERP_CONE   — bilinear cone-tangent sampling via texture(); default: texelFetch (nearest)
//   Both, one, or neither may be defined independently.
//   The Texture(w,h,GL_RGBA8) constructor sets GL_LINEAR, so texture() gives bilinear.
//
// Linear search height sampler:
//   By default ls_sampleHeight reads from coneMap.r so the geometry shader debug
//   pass can include this file without change.  Fragment shaders that want to use
//   the raw heightmap instead define the macro before including:
//
//     #define LS_SAMPLE_HEIGHT(uv) texture(texImage, uv).r
//     #include "RayMarch_common.glsl"
//
// Debug SSBO writes:
//   The geometry shader defines DEBUG_FUNCTIONS before including this file.
//   This enables SSBO declarations (binding 0 + 1) and the DBG_WRITE_STEP macro.
//   The fragment shader does not define DEBUG_FUNCTIONS, so DBG_WRITE_STEP
//   expands to nothing there — no SSBO declarations, no writes.

// ---------------------------------------------------------------------------
// Debug SSBO (compiled only when DEBUG_FUNCTIONS is defined, i.e. geometry shader)
// ---------------------------------------------------------------------------

#ifdef DEBUG_FUNCTIONS
layout(std430, binding = 0) buffer DebugVisualSSBO { vec4 debugVisual[]; };

// debugNumericalSSBO layout:
//   indirectSteps  (uvec4, offset   0): {stepCount,   1, 0, 0} — GL_DRAW_INDIRECT_BUFFER for steps
//   indirectCones  (uvec4, offset  16): {3, stepCount, 0, 0} — GL_DRAW_INDIRECT_BUFFER for cones (instanced LINE_STRIP)
//   debugNumerical (vec4[], offset 32): header[0..22] + per-step data[23+]
layout(std430, binding = 1) buffer DebugNumericalSSBO {
    uvec4 indirectSteps;
    uvec4 indirectCones;
    vec4  debugNumerical[];
};

#define SSBO_PADDING 9

int g_debugStepIdx = 0;

#define DBG_WRITE_STEP(ui_pos, ti_val, t_val, height_val, tan_val) \
    if (g_debugStepIdx < 512) { \
        debugVisual[SSBO_PADDING + g_debugStepIdx] = vec4(ui_pos, 1.0); \
        debugNumerical[23 + g_debugStepIdx * 2]     = vec4(ti_val, t_val, height_val, tan_val); \
        debugNumerical[23 + g_debugStepIdx * 2 + 1] = vec4(ui_pos, 0.0); \
        ++g_debugStepIdx; \
    }
#else
#define DBG_WRITE_STEP(ui_pos, ti_val, t_val, height_val, tan_val)
#endif

// ---------------------------------------------------------------------------
// Structs
// ---------------------------------------------------------------------------

struct UnitIntersection {
    bool  found;
    float near;
    float far;
};

struct Ray {
    vec3 start;
    vec3 dir;
};

struct IntersectParams {
    vec3 enter;
    vec3 exit;
    vec3 cam;
};

struct IntersectReturn {
    vec2  uv;
    float t;
    float last_t;
    bool  wasHit;
    int   flags;
    int   stepCount;
};

struct StepParams {
    vec3 point;
    Ray  view;
    vec2 tex;
};

struct StepReturn {
    float t;
};

// ---------------------------------------------------------------------------
// Conemap sampling
// ---------------------------------------------------------------------------

// .r = height, .g = tangent of cone half-angle
// Each channel is sampled independently:
//   INTERP_HEIGHT → bilinear height  (texture()); otherwise nearest (texelFetch)
//   INTERP_CONE   → bilinear tangent (texture()); otherwise nearest (texelFetch)
vec2 conemap_get(vec2 uv) {
#ifdef INTERP_HEIGHT
    float h = texture(coneMap, uv).r;
#else
    float h = texelFetch(coneMap, clamp(ivec2(uv * vec2(textureSize(coneMap, 0))),
                                        ivec2(0), textureSize(coneMap, 0) - 1), 0).r;
#endif

#ifdef INTERP_CONE
    float t = texture(coneMap, uv).g;
#else
    float t = texelFetch(coneMap, clamp(ivec2(uv * vec2(textureSize(coneMap, 0))),
                                        ivec2(0), textureSize(coneMap, 0) - 1), 0).g;
#endif

    return vec2(h, t);
}

float conemap_getHeight(vec2 uv) {
    return conemap_get(uv).r;
}

// Finite-difference normal from the conemap height channel
vec3 getNormalFromHeightmap(vec2 uv) {
    const float eps = 0.001;
    float h  = conemap_getHeight(uv);
    float hx = conemap_getHeight(uv + vec2(eps, 0.0));
    float hy = conemap_getHeight(uv + vec2(0.0, eps));
    vec3 dx = vec3(eps, 0.0, hx - h);
    vec3 dy = vec3(0.0, eps, hy - h);
    return normalize(cross(dx, dy));
}

// ---------------------------------------------------------------------------
// Linear search
// ---------------------------------------------------------------------------

// Height sampler used by findIntersection_linearSearch and ls_sampleNormal.
// Override by defining LS_SAMPLE_HEIGHT before including this file.
#ifndef LS_SAMPLE_HEIGHT
#define LS_SAMPLE_HEIGHT(uv) conemap_getHeight(uv)
#endif

vec3 ls_sampleNormal(vec2 uv) {
    const float eps = 0.001;
    float h  = LS_SAMPLE_HEIGHT(uv);
    float hx = LS_SAMPLE_HEIGHT(uv + vec2(eps, 0.0));
    float hy = LS_SAMPLE_HEIGHT(uv + vec2(0.0, eps));
    vec3 dx = vec3(eps, 0.0, hx - h);
    vec3 dy = vec3(0.0, eps, hy - h);
    return normalize(cross(dx, dy));
}

IntersectReturn findIntersection_linearSearch(IntersectParams params) {
    vec3  v         = params.exit - params.enter;
    float step      = 1.0 / float(maxSteps);
    int   stepCount = 0;

    for (float t = 0.0; t <= 1.0 + step && stepCount <= maxSteps; t += step) {
        vec3  u = params.enter + t * v;
        float h = LS_SAMPLE_HEIGHT(u.xy);
        DBG_WRITE_STEP(u, step, t, h, 0.0)
        if (h > u.z)
            return IntersectReturn(u.xy, t, t, true, 0, stepCount);
        ++stepCount;
    }
    return IntersectReturn(vec2(0.0), 0.0, 0.0, false, 0, stepCount);
}

// ---------------------------------------------------------------------------
// Cone step
// ---------------------------------------------------------------------------

StepReturn getNextStep(StepParams params) {
    vec3  u    = params.point;
    vec3  v    = params.view.dir;
    float az   = params.tex.r;
    float ctga = 1.0 / params.tex.g;
    float dz   = az - u.z;
    float horiz = length(v.xy);

    // Safe advance: t * horiz = (w-h + t*v.z) * tan_val  →  t = dz / (v.z - horiz*ctga)
    // The other root (v.z + horiz*ctga) is the mirrored half of the double cone — wrong direction.
    float denom = v.z - horiz * ctga;
    StepReturn val;
    val.t = abs(denom) > 1e-7 ? dz / denom : -1.0;
    return val;
}

// ---------------------------------------------------------------------------
// Cone step mapping
// ---------------------------------------------------------------------------

IntersectReturn findIntersection_coneStepMapping(IntersectParams params) {
    vec3  u1  = params.enter;
    vec3  u2  = params.exit;
    vec3  e   = params.cam;
    int   stepCount = 0;
    int   flags     = 0;

    // Pre-check: already below the surface at entry
    if (conemap_getHeight(u1.xy) >= u1.z) {
        return IntersectReturn(u1.xy, 0.0, 0.0, true, 0, 0);
    }

    vec3 v = u1 - e;    // direction from camera to entry point

    // t at exit point along ray e + t*v
    float maxT = 1e7;
    if      (abs(v.x) > 1e-4) maxT = (u2.x - e.x) / v.x;
    else if (abs(v.y) > 1e-4) maxT = (u2.y - e.y) / v.y;
    else                       maxT = (u2.z - e.z) / v.z;

    float t  = 1.000001;
    vec3  ui = e + t * v;
    vec2  tex = conemap_get(ui.xy);
    float last_t = 0.0;
    float ti = t + 1.0;

    // Record the entry point cone before the first step (ti=0 = no step taken yet)
    DBG_WRITE_STEP(ui, 0.0, t, tex.r, tex.g)

    while (stepCount <= maxSteps && t < maxT && ti > 1e-6) {
        StepReturn stepData = getNextStep(StepParams(ui, Ray(e, v), tex));
        ti = stepData.t;

        if (ti < 0.0) {
            return IntersectReturn(vec2(0.0), -1.0, -1.0, false, 0, 0);
        }

        last_t = t;
        t     += ti;
        ui     = e + t * v;
        tex    = conemap_get(ui.xy);
        DBG_WRITE_STEP(ui, ti, t, tex.r, tex.g)
        ++stepCount;

        if (ui.z <= tex.r) {
            ti = -1.0;
            break;
        }
    }

    flags = int(stepCount > maxSteps)
          | (int(t >= maxT)    << 1)
          | (int(ti <= 1e-6)   << 2);

    bool wasHit = bool(flags & 4) && !bool(flags & 1);
    return IntersectReturn(ui.xy, t, last_t, wasHit, flags, stepCount);
}

// ---------------------------------------------------------------------------
// Unit prism intersection (for computing entry/exit in texture space)
// ---------------------------------------------------------------------------

UnitIntersection intersectUnitPrism(Ray ray) {
    float n = -1e10, f = 1e10;
    vec3 p0 = ray.start;
    vec3 v  = ray.dir;
    vec3 t0 = -p0 / v;

    if      (v.x > 0.0)  { n = max(n, t0.x); }
    else if (v.x < 0.0)  { f = min(f, t0.x); }
    else if (p0.x < 0.0) { return UnitIntersection(false, 0.0, 0.0); }

    if      (v.y > 0.0)  { n = max(n, t0.y); }
    else if (v.y < 0.0)  { f = min(f, t0.y); }
    else if (p0.y < 0.0) { return UnitIntersection(false, 0.0, 0.0); }

    if      (v.z > 0.0)  { n = max(n, t0.z); }
    else if (v.z < 0.0)  { f = min(f, t0.z); }
    else if (p0.z < 0.0) { return UnitIntersection(false, 0.0, 0.0); }

    // Top plane (y = 1)
    vec3  q  = vec3(1.0, 1.0, 0.0) - p0;
    float t1 = q.y / v.y;
    if (v.y < 0.0) { n = max(n, t1); } else { f = min(f, t1); }

    // Diagonal plane (x + z = 1)
    float t2 = (q.x + q.z) / (v.x + v.z);
    if (v.x + v.z < 0.0) { n = max(n, t2); } else { f = min(f, t2); }

    return UnitIntersection(n < f, n, f);
}
