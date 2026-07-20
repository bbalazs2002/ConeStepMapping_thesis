#include "Material/Material.h"
#include "Texture/Texture.h"

#include <functional>

Material::~Material() = default;

// -- Equality and hashing ------------------------------------------------------

bool Material::operator==(const Material& other) const
{
    return m_name          == other.m_name
        && m_ambientColor  == other.m_ambientColor
        && m_diffuseColor  == other.m_diffuseColor
        && m_specularColor == other.m_specularColor
        && m_shininess     == other.m_shininess
        && m_diffuseTex    == other.m_diffuseTex    // shared_ptr equality (pointer identity)
        && m_specularTex   == other.m_specularTex
        && m_emissionTex   == other.m_emissionTex
        && m_normalTex     == other.m_normalTex;
}

static std::size_t hashTex(const std::shared_ptr<Texture>& t)
{
    GLuint id = t ? t->GetID() : 0u;
    return std::hash<GLuint>{}(id);
}

std::size_t Material::Hash(const Material& mat)
{
    std::size_t seed = 0;
    auto combine = [&seed](std::size_t h) {
        seed ^= h + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    };

    combine(std::hash<std::string>{}(mat.m_name));
    combine(std::hash<float>{}(mat.m_ambientColor.r));
    combine(std::hash<float>{}(mat.m_ambientColor.g));
    combine(std::hash<float>{}(mat.m_ambientColor.b));
    combine(std::hash<float>{}(mat.m_diffuseColor.r));
    combine(std::hash<float>{}(mat.m_diffuseColor.g));
    combine(std::hash<float>{}(mat.m_diffuseColor.b));
    combine(std::hash<float>{}(mat.m_specularColor.r));
    combine(std::hash<float>{}(mat.m_specularColor.g));
    combine(std::hash<float>{}(mat.m_specularColor.b));
    combine(std::hash<float>{}(mat.m_shininess));
    combine(hashTex(mat.m_diffuseTex));
    combine(hashTex(mat.m_specularTex));
    combine(hashTex(mat.m_emissionTex));
    combine(hashTex(mat.m_normalTex));

    return seed;
}

// -- Shader upload (DSA) -------------------------------------------------------

static GLuint texID(const std::shared_ptr<Texture>& t) { return t ? t->GetID() : 0u; }

void Material::UploadMaterialToShader(
    GLuint programID,
    const std::shared_ptr<Material>& mat,
    GLuint textureTargets[4]
)
{
    ClearMaterialFromShader();

    std::copy(textureTargets, textureTargets + 4, s_lastTextureTargets);
    s_hasUploadedData = true;

    auto setVec4 = [&](const char* name, const glm::vec4& v) {
        GLint loc = glGetUniformLocation(programID, name);
        if (loc >= 0) glUniform4fv(loc, 1, &v[0]);
    };
    auto setFloat = [&](const char* name, float value) {
        GLint loc = glGetUniformLocation(programID, name);
        if (loc >= 0) glUniform1f(loc, value);
    };
    auto setInt = [&](const char* name, int value) {
        GLint loc = glGetUniformLocation(programID, name);
        if (loc >= 0) glUniform1i(loc, value);
    };

    setVec4("materialData.diffuseColorTex",
        glm::vec4(mat->GetDiffuseColor(),  static_cast<float>(texID(mat->GetDiffuseTex()))));
    setVec4("materialData.specularColorTex",
        glm::vec4(mat->GetSpecularColor(), static_cast<float>(texID(mat->GetSpecularTex()))));
    setVec4("materialData.ambientColorEmissionTex",
        glm::vec4(mat->GetAmbientColor(),  static_cast<float>(texID(mat->GetEmissionTex()))));
    setFloat("materialData.shininess", mat->GetShininess());
    setInt("materialData.hasNormalTex", texID(mat->GetNormalTex()) != 0 ? 1 : 0);

    // DSA: glBindTextureUnit replaces glActiveTexture + glBindTexture
    if (texID(mat->GetDiffuseTex())  != 0) {
        glBindTextureUnit(textureTargets[0], texID(mat->GetDiffuseTex()));
        setInt("materialDiffuseTex",  static_cast<int>(textureTargets[0]));
    }
    if (texID(mat->GetSpecularTex()) != 0) {
        glBindTextureUnit(textureTargets[1], texID(mat->GetSpecularTex()));
        setInt("materialSpecularTex", static_cast<int>(textureTargets[1]));
    }
    if (texID(mat->GetEmissionTex()) != 0) {
        glBindTextureUnit(textureTargets[2], texID(mat->GetEmissionTex()));
        setInt("materialEmissionTex", static_cast<int>(textureTargets[2]));
    }
    if (texID(mat->GetNormalTex())   != 0) {
        glBindTextureUnit(textureTargets[3], texID(mat->GetNormalTex()));
        setInt("materialNormalTex",   static_cast<int>(textureTargets[3]));
    }
}

void Material::UploadMaterialToShader(
    GLuint programID,
    const std::shared_ptr<Material>& material
)
{
    GLuint defaultTargets[] = { 0, 1, 2, 3 };
    UploadMaterialToShader(programID, material, defaultTargets);
}

void Material::ClearMaterialFromShader()
{
    if (!s_hasUploadedData) return;
    for (int i = 0; i < 4; ++i)
        glBindTextureUnit(s_lastTextureTargets[i], 0);
}
