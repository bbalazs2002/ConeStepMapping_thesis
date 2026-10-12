// This file may contain AI-assisted (Claude Code) code, reviewed by the author.

struct MaterialUniforms {
    vec4 diffuseColorTex;           // xyz: diffuse color, w: has diffuse texture
    vec4 specularColorTex;          // xyz: specular color, w: has specular texture
    vec4 ambientColorEmissionTex;   // xyz: ambient color, w: has emission texture
    float shininess;
    int hasNormalTex;
};
uniform sampler2D materialDiffuseTex;
uniform sampler2D materialSpecularTex;
uniform sampler2D materialEmissionTex;
uniform sampler2D materialNormalTex;

uniform MaterialUniforms materialData;
