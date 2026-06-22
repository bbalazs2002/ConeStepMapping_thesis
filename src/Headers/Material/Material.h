#pragma once

#include <string>
#include <filesystem>
#include <memory>

#include <GL/glew.h>
#include <glm/glm.hpp>

class Material {
public:
    Material() = default;

    // Destructor releases all GPU texture handles
    ~Material();

    // ── Getters / Setters (inline) ────────────────────────────────────────────

    inline const std::string& GetName() const { return m_name; }
    inline void SetName(const std::string& name) { m_name = name; }

    inline const glm::vec3& GetAmbientColor() const { return m_ambientColor; }
    inline void SetAmbientColor(const glm::vec3& color) { m_ambientColor = color; }

    inline const glm::vec3& GetDiffuseColor() const { return m_diffuseColor; }
    inline void SetDiffuseColor(const glm::vec3& color) { m_diffuseColor = color; }

    inline const glm::vec3& GetSpecularColor() const { return m_specularColor; }
    inline void SetSpecularColor(const glm::vec3& color) { m_specularColor = color; }

    inline float GetShininess() const { return m_shininess; }
    inline void SetShininess(float shininess) { m_shininess = shininess; }

    inline GLuint GetDiffuseTex() const { return m_diffuseTex; }
    inline void SetDiffuseTex(GLuint tex) { m_diffuseTex = tex; }

    inline GLuint GetSpecularTex() const { return m_specularTex; }
    inline void SetSpecularTex(GLuint tex) { m_specularTex = tex; }

    inline GLuint GetEmissionTex() const { return m_emissionTex; }
    inline void SetEmissionTex(GLuint tex) { m_emissionTex = tex; }

    inline GLuint GetNormalTex() const { return m_normalTex; }
    inline void SetNormalTex(GLuint tex) { m_normalTex = tex; }

    // ── Equality and hashing (for MaterialManager) ────────────────────────────

    bool operator==(const Material& other) const;

    static std::size_t Hash(const Material& mat);

    // ── Static utilities ──────────────────────────────────────────────────────

    /**
     * @brief Loads an image from a file, creates an OpenGL texture object,
     *        and configures its parameters (mipmaps, filtering, wrapping).
     *
     * @param path     Filesystem path to the image file.
     * @param flip     Whether to flip the image vertically on load.
     * @return         OpenGL texture ID, or 0 on failure.
     */
    static GLuint LoadTexture(const std::filesystem::path& path, bool flip = false);

    /**
     * @brief Uploads material properties and binds textures to the active shader program.
     *
     * Packs color and texture handles into vec4 structures (materialData) to reduce
     * uniform overhead. The shader program must be active before calling this function.
     *
     * @param programID      Handle of the active shader program.
     * @param material       Material to upload.
     * @param textureTargets Texture unit indices for [Diffuse, Specular, Emission, Normal].
     */
    static void UploadMaterialToShader(
        GLuint programID,
        const std::shared_ptr<Material>& material,
        GLuint textureTargets[4]
    );

    // Overload using default texture units [0, 1, 2, 3]
    static void UploadMaterialToShader(
        GLuint programID,
        const std::shared_ptr<Material>& material
    );

    /**
     * @brief Unbinds all material textures from their texture units.
     *
     * Prevents stale texture state from affecting subsequent draw calls.
     */
    static void ClearMaterialFromShader();

private:
    std::string m_name = "default";

    glm::vec3 m_ambientColor = glm::vec3(1.0f);
    glm::vec3 m_diffuseColor = glm::vec3(1.0f);
    glm::vec3 m_specularColor = glm::vec3(1.0f);

    float m_shininess = 32.0f;

    GLuint m_diffuseTex = 0;
    GLuint m_specularTex = 0;
    GLuint m_emissionTex = 0;
    GLuint m_normalTex = 0;

    // Stores the last texture units used by UploadMaterialToShader,
    // so ClearMaterialFromShader knows which units to unbind.
    inline static GLuint s_lastTextureTargets[4] = {0, 1, 2, 3};
};