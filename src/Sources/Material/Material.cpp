#include "Material/Material.h"

#include <functional>

#include <tiny_obj_loader.h>

#include "GLUtils.hpp"
#include "Log.h"

// ── Destructor ────────────────────────────────────────────────────────────────

Material::~Material()
{
    // NOTE: glDeleteTextures silently ignores IDs of 0, so no need to check.
    glDeleteTextures(1, &m_diffuseTex);  m_diffuseTex = 0;
    glDeleteTextures(1, &m_specularTex); m_specularTex = 0;
    glDeleteTextures(1, &m_emissionTex); m_emissionTex = 0;
    glDeleteTextures(1, &m_normalTex);   m_normalTex = 0;
}

// ── Equality and hashing ──────────────────────────────────────────────────────

bool Material::operator==(const Material& other) const
{
    return m_name == other.m_name
        && m_ambientColor == other.m_ambientColor
        && m_diffuseColor == other.m_diffuseColor
        && m_specularColor == other.m_specularColor
        && m_shininess == other.m_shininess
        && m_diffuseTex == other.m_diffuseTex
        && m_specularTex == other.m_specularTex
        && m_emissionTex == other.m_emissionTex
        && m_normalTex == other.m_normalTex;
}

std::size_t Material::Hash(const Material& mat)
{
    // Combine hashes of all relevant fields using a standard seed mixing strategy
    std::size_t seed = 0;

    auto combine = [&seed](std::size_t hash) {
        seed ^= hash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
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
    combine(std::hash<GLuint>{}(mat.m_diffuseTex));
    combine(std::hash<GLuint>{}(mat.m_specularTex));
    combine(std::hash<GLuint>{}(mat.m_emissionTex));
    combine(std::hash<GLuint>{}(mat.m_normalTex));

    return seed;
}

// ── Static utilities ──────────────────────────────────────────────────────────

GLuint Material::LoadTexture(const std::filesystem::path& path, bool flip)
{
    if (path.empty()) return 0;

    ImageRGBA img = ImageFromFile(path, flip);
    if (img.width <= 0 || img.height <= 0) {
        LOG_ERROR("[Material] Failed to load texture: ", path.generic_string());
        return 0;
    }

    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.width, img.height,
        0, GL_RGBA, GL_UNSIGNED_BYTE, img.data());
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);

    return texID;
}

void Material::UploadMaterialToShader(
    GLuint programID,
    const std::shared_ptr<Material>& material,
    GLuint textureTargets[4]
)
{
    // Save texture targets for ClearMaterialFromShader
    memcpy(s_lastTextureTargets, textureTargets, sizeof(GLuint) * 4);

    // ── Uniform helper lambdas ────────────────────────────────────────────────
    auto setVec4 = [&](const std::string& name, const glm::vec4& v) {
        GLint loc = glGetUniformLocation(programID, name.c_str());
        if (loc >= 0) glUniform4fv(loc, 1, &v[0]);
    };
    auto setFloat = [&](const std::string& name, float value) {
        GLint loc = glGetUniformLocation(programID, name.c_str());
        if (loc >= 0) glUniform1f(loc, value);
    };
    auto setInt = [&](const std::string& name, int value) {
        GLint loc = glGetUniformLocation(programID, name.c_str());
        if (loc >= 0) glUniform1i(loc, value);
    };

    // ── Upload color and scalar uniforms (materialData struct) ────────────────
    setVec4("materialData.diffuseColorTex",
        glm::vec4(material->GetDiffuseColor(), material->GetDiffuseTex()));
    setVec4("materialData.specularColorTex",
        glm::vec4(material->GetSpecularColor(), material->GetSpecularTex()));
    setVec4("materialData.ambientColorEmissionTex",
        glm::vec4(material->GetAmbientColor(), material->GetEmissionTex()));
    setFloat("materialData.shininess", material->GetShininess());
    setInt("materialData.hasNormalTex", material->GetNormalTex());

    // ── Bind textures to their respective texture units ───────────────────────

    // 1. Diffuse map
    if (material->GetDiffuseTex() > 0) {
        glActiveTexture(GL_TEXTURE0 + textureTargets[0]);
        glBindTexture(GL_TEXTURE_2D, material->GetDiffuseTex());
        setInt("materialDiffuseTex", textureTargets[0]);
    }
    // 2. Specular map
    if (material->GetSpecularTex() > 0) {
        glActiveTexture(GL_TEXTURE0 + textureTargets[1]);
        glBindTexture(GL_TEXTURE_2D, material->GetSpecularTex());
        setInt("materialSpecularTex", textureTargets[1]);
    }
    // 3. Emission map
    if (material->GetEmissionTex() > 0) {
        glActiveTexture(GL_TEXTURE0 + textureTargets[2]);
        glBindTexture(GL_TEXTURE_2D, material->GetEmissionTex());
        setInt("materialEmissionTex", textureTargets[2]);
    }
    // 4. Normal map
    if (material->GetNormalTex() > 0) {
        glActiveTexture(GL_TEXTURE0 + textureTargets[3]);
        glBindTexture(GL_TEXTURE_2D, material->GetNormalTex());
        setInt("materialNormalTex", textureTargets[3]);
    }

    // Restore active texture unit to GL_TEXTURE0 for safety
    glActiveTexture(GL_TEXTURE0);
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
    // Unbind all textures from their last used texture units
    for (int i = 0; i < 4; ++i) {
        glActiveTexture(GL_TEXTURE0 + s_lastTextureTargets[i]);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // Restore active texture unit to GL_TEXTURE0 for safety
    glActiveTexture(GL_TEXTURE0);
}