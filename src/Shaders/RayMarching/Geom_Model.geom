#version 430

// Input: a single triangle (3 vertices)
layout(triangles) in;

// Output: a quad (as a triangle strip with 4 vertices) representing the bounding box
layout(triangle_strip, max_vertices = 4) out;

// Input attributes from the vertex shader
in vec3 vs_out_fragPos[];   // World-space position for each vertex
in vec3 vs_out_norm[];      // World-space normal for each vertex
in vec2 vs_out_tex[];       // Texture coordinates
in vec3 vs_out_viewDir[];   // View direction (from vertex to camera)

// Outputs to the fragment shader
out vec3 gs_out_fragPos;
out vec3 gs_out_norm;
out vec2 gs_out_tex;
out vec3 gs_out_viewDir;

// Uniforms
uniform mat4 viewProj;        // View-projection matrix
uniform float maxDisplacement; // Maximum displacement along the normal

void main() {
    // 1. Compute "displaced" positions for each vertex (simulate maximum height displacement)
    vec3 displaced[3];
    for (int i = 0; i < 3; ++i) {
        displaced[i] = vs_out_fragPos[i] + vs_out_norm[i] * maxDisplacement;
    }
    
    // 2. Gather both the original and displaced vertices into one array.
    vec3 points[6];
    for (int i = 0; i < 3; ++i) {
        points[i]   = vs_out_fragPos[i];
        points[i+3] = displaced[i];
    }
    
    // 3. Compute an axis-aligned bounding box (AABB) that encloses all these points.
    vec3 bbMin = points[0];
    vec3 bbMax = points[0];
    for (int i = 1; i < 6; ++i) {
        bbMin = min(bbMin, points[i]);
        bbMax = max(bbMax, points[i]);
    }
    
    // 4. Define the four corners of the AABB.
    //    (These corners are in world space.)
    vec3 corner0 = vec3(bbMin.x, bbMin.y, bbMin.z);
    vec3 corner1 = vec3(bbMax.x, bbMin.y, bbMin.z);
    vec3 corner2 = vec3(bbMax.x, bbMax.y, bbMax.z);
    vec3 corner3 = vec3(bbMin.x, bbMax.y, bbMax.z);
    
    // 5. For the bounding box, we can use an average normal from the triangle.
    vec3 avgNorm = normalize(vs_out_norm[0] + vs_out_norm[1] + vs_out_norm[2]);
    
    // 6. Set up texture coordinates for the quad.
    vec2 uv0 = vec2(0.0, 0.0);
    vec2 uv1 = vec2(1.0, 0.0);
    vec2 uv2 = vec2(1.0, 1.0);
    vec2 uv3 = vec2(0.0, 1.0);
    
    // 7. Choose a view direction to pass along.
    //    (For example, use the view direction from the first vertex.)
    vec3 viewDir = vs_out_viewDir[0];
    
    // 8. Emit the four vertices as a triangle strip.
    //    Order them so that the strip covers the quad.
    vec3 corners[4] = { corner0, corner1, corner3, corner2 };
    vec2 uvs[4]     = { uv0, uv1, uv3, uv2 };
    
    for (int i = 0; i < 4; ++i) {
        gl_Position = viewProj * vec4(corners[i], 1.0);
        gs_out_fragPos = corners[i];
        gs_out_norm    = avgNorm;
        gs_out_tex     = uvs[i];
        gs_out_viewDir = viewDir;
        EmitVertex();
    }
    
    EndPrimitive();
}