// This file may contain AI-assisted (Claude Code) code, reviewed by the author.

float[13] MaterialPrepare(vec2 texCoord) {
    // --- Material Preparation (Texture Sampling) ---
    vec3 diffuseColor = materialData.diffuseColorTex.xyz;
    if (materialData.diffuseColorTex.w > 0.5) {
        diffuseColor = texture(materialDiffuseTex, texCoord).rgb;
    }

    vec3 specularColor = materialData.specularColorTex.xyz;
    if (materialData.specularColorTex.w > 0.5) {
        specularColor = texture(materialSpecularTex, texCoord).rgb;
    }

    vec3 ambientColor = diffuseColor * materialData.ambientColorEmissionTex.xyz;
    vec3 emissionColor = vec3(0.0);
    if (materialData.ambientColorEmissionTex.w > 0.5) {
        emissionColor = texture(materialEmissionTex, texCoord).rgb;
    }
    if (emissionColor.x > 0.f || emissionColor.y > 0.f || emissionColor.z > 0.f) {
        ambientColor = vec3(0);
        diffuseColor = vec3(0);
        specularColor = vec3(0);
    }

    // --- Packing into float[12]
    float[13] value = float[13](
        // ambientColor
        ambientColor.r, ambientColor.g, ambientColor.b,
        // diffuseColor
        diffuseColor.r, diffuseColor.g, diffuseColor.b,
        // specularColor
        specularColor.r, specularColor.g, specularColor.b,
        // emissionColor
        emissionColor.r, emissionColor.g, emissionColor.b,
        // shininess
        materialData.shininess
    );

    return value;
}