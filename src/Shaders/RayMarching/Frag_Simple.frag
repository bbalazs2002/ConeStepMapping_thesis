#version 430 core

// Inputs from the geometry shader
in vec2 gs_out_tex;
in vec3 gs_out_norm;
in vec3 gs_out_pos;

// Final output color
out vec4 fs_out_col;

// Uniform samplers
uniform sampler2D texImage;  // The base color texture
uniform sampler2D conemap;   // The cone map: r = minimum tangent (step size), g = height value

// Uniform camera position (in world space)
uniform vec3 camPos;

// Maximum number of cone step mapping iterations
const int MAX_STEPS = 20;

// Perform cone step mapping on the texture coordinate
vec2 performConeStepMapping(vec2 uv, vec3 viewDir)
{
    // Normalize the view direction’s XY component.
    // (Assumes that your texture coordinates are aligned with the surface tangent space.)
    vec2 stepDir = normalize(viewDir.xy);
    
    // This variable accumulates the total step length in texture space.
    float totalStep = 0.0;
    
    // Iteratively march along the view ray in texture space.
    for (int i = 0; i < MAX_STEPS; i++) {
        // Compute a new candidate texture coordinate.
        vec2 sampleUV = uv + totalStep * stepDir;
        
        // Sample the cone map at the candidate coordinate.
        // - coneData.r is the minimum tangent (step size).
        // - coneData.g is the height value.
        vec4 coneData = texture(conemap, sampleUV);
        float coneStep = coneData.r;
        float coneHeight = coneData.g;
        
        // If the distance we've traveled exceeds the sampled height, we assume we've hit the surface.
        if (totalStep > coneHeight)
            break;
        
        // Increase the step by the step size from the cone map.
        totalStep += coneStep;
    }
    
    // Return the displaced texture coordinate.
    return uv + totalStep * stepDir;
}

void main()
{
    // Compute the view direction from the fragment to the camera.
    // (Typically, this should be transformed into the same space as your texture mapping.)
    vec3 viewDir = normalize(camPos - gs_out_pos);

    // Compute new texture coordinates using cone step mapping.
    vec2 displacedUV = performConeStepMapping(gs_out_tex, viewDir);

    // Sample the base texture using the displaced UV.
    vec4 baseColor = texture(texImage, displacedUV);

    // Optionally, apply simple diffuse lighting based on the surface normal.
    float diffuse = max(dot(normalize(gs_out_norm), normalize(viewDir)), 0.0);
    
    // Output the final color.
    fs_out_col = baseColor * diffuse;
}