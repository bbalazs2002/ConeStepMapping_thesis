struct LightCalculateContributionParams {
    Light light;
    vec3 position;      // fragment position
    vec3 norm;          // normalised
    vec3 viewDir;       // normalised
    vec3 diffuseColor;
    vec3 specularColor;
    float shininess;
};
vec3 LightCalculateContribution(LightCalculateContributionParams params) {
    vec3 lightDir;
    float attenuation = 1.0;
    float spotIntensity = 1.0;
    uint flags = uint(params.light.flags_angle_plane.x);

    // 1. Determine Light Direction and Attenuation
    if ((LIGHT_FLAG_IS_DIR & flags) != 0u) {
        lightDir = normalize(-params.light.direction.xyz);
    }
    else { // Point or Spot Light
        vec3 fragToLight = params.light.position.xyz - params.position;
        float dist = length(fragToLight);
        lightDir = normalize(fragToLight);

        // Attenuation calculation (constant, linear, quadratic)
        attenuation = (params.light.La_const.w + params.light.Ld_linear.w * dist + params.light.Ls_quadratic.w * dist * dist);

        if ((LIGHT_FLAG_IS_SPOT & flags) != 0u) {
            // Spot Light Calculation
            vec3 spotDir = normalize(params.light.direction.xyz);
            float theta = dot(lightDir, -spotDir);      // cosine of angle between light ray and spot direction

            float innerCutOff = cos(params.light.flags_angle_plane.y);
            float outerCutOff = cos(params.light.flags_angle_plane.z);

            if (theta > outerCutOff) {
                // Smooth fade from inner to outer cutoff (soft edges)
                spotIntensity = smoothstep(outerCutOff, innerCutOff, theta);
            } else {
                // Fragment is outside the spot cone
                spotIntensity = 0.0;
            }
        }
    }

    // If the light is dimmed out by spot or attenuation, skip the expensive calculations
    if (attenuation <= 0.0 || spotIntensity <= 0.0) {
        return vec3(0.0);
    }

    // Surface facing away from the light receives no contribution
    float NdotL = dot(params.norm, lightDir);
    if (NdotL <= 0.0) {
        return vec3(0.0);
    }

    // 2. Diffuse Component
    vec3 diffuse = params.light.Ld_linear.xyz * params.diffuseColor * NdotL;

    // 3. Specular Component (Blinn-Phong)
    vec3 halfDir = normalize(lightDir + params.viewDir);
    float spec = pow(max(dot(params.norm, halfDir), 0.0), params.shininess);
    vec3 specular = params.light.Ls_quadratic.xyz * params.specularColor * spec;

    // 4. Combine and apply attenuation/spot factor
    return ((diffuse + specular) * spotIntensity) / attenuation;
}

struct LightCalculateParams{
    vec3 norm;          // normal vector at the fragment
    vec3 viewDir;       // fragment to camera direction
    vec3 position;      // fragment position in world space
    float[13] material; // material properties packed as follows:
                        //  [0-2]:  ambient color
                        //  [3-5]:  diffuse color
                        //  [6-8]:  specular color
                        //  [9-11]: emission color
                        //  [12]:   shininess
};
vec3 LightCalculate(LightCalculateParams params) {
    // normalise vectors
    vec3 norm = normalize(params.norm);
    vec3 viewDir = normalize(params.viewDir);

    // unpack material properties
    vec3 ambientColor  = vec3(params.material[0],  params.material[1], params.material[2]);
    vec3 diffuseColor  = vec3(params.material[3],  params.material[4], params.material[5]);
    vec3 specularColor = vec3(params.material[6],  params.material[7], params.material[8]);
    vec3 emissionColor = vec3(params.material[9],  params.material[10], params.material[11]);
    float shininess    = params.material[12];

    // --- 1. Calculate Ambient Light (The base color component) ---
    // Start with the base ambient contribution, derived from the material's ambient color
    vec3 totalLight = ambientColor;

    // --- 2. Iterate and Accumulate Light Contributions ---
    for (int i = 0; i < lightData.lightCount; i++) {
        // Ambient light from the light source itself (La_const.xyz)
        totalLight += lightSources[i].La_const.xyz * ambientColor;

        // Diffuse and Specular components
        totalLight += LightCalculateContribution(LightCalculateContributionParams(
            lightSources[i],
            params.position,
            norm, viewDir,
            diffuseColor, specularColor,
            shininess
        ));
    }

    return max(vec3(0), min(vec3(1.f), totalLight + emissionColor));
}
