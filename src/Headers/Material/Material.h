#pragma once

#include <string>
#include <memory>

#include <GL/glew.h>
#include <glm/glm.hpp>

class Texture;  // forward declaration

class Material {
public:
    Material() = default;
    ~Material();  // = default in .cpp

    // -- Getters / Setters -----------------------------------------------------

    inline const std::string& GetName() const { return m_name; }
    inline void SetName(const std::string& name) { m_name = name; }

    inline const glm::vec3& GetAmbientColor()  const { return m_ambientColor; }
    inline void SetAmbientColor(const glm::vec3& c)  { m_ambientColor = c; }

    inline const glm::vec3& GetDiffuseColor()  const { return m_diffuseColor; }
    inline void SetDiffuseColor(const glm::vec3& c)  { m_diffuseColor = c; }

    inline const glm::vec3& GetSpecularColor() const { return m_specularColor; }
    inline void SetSpecularColor(const glm::vec3& c) { m_specularColor = c; }

    inline float GetShininess() const { return m_shininess; }
    inline void  SetShininess(float s) { m_shininess = s; }

    inline const std::shared_ptr<Texture>& GetDiffuseTex()  const { return m_diffuseTex; }
    inline const std::shared_ptr<Texture>& GetSpecularTex() const { return m_specularTex; }
    inline const std::shared_ptr<Texture>& GetEmissionTex() const { return m_emissionTex; }
    inline const std::shared_ptr<Texture>& GetNormalTex()   const { return m_normalTex; }

    inline void SetDiffuseTex (std::shared_ptr<Texture> t) { m_diffuseTex  = std::move(t); }
    inline void SetSpecularTex(std::shared_ptr<Texture> t) { m_specularTex = std::move(t); }
    inline void SetEmissionTex(std::shared_ptr<Texture> t) { m_emissionTex = std::move(t); }
    inline void SetNormalTex  (std::shared_ptr<Texture> t) { m_normalTex   = std::move(t); }

    // -- Equality and hashing --------------------------------------------------

    bool operator==(const Material& other) const;
    static std::size_t Hash(const Material& mat);

    // -- Shader upload ---------------------------------------------------------

    static void UploadMaterialToShader(
        GLuint programID,
        const std::shared_ptr<Material>& material,
        GLuint textureTargets[4]
    );
    static void UploadMaterialToShader(
        GLuint programID,
        const std::shared_ptr<Material>& material
    );
    static void ClearMaterialFromShader();

protected:
    std::string m_name = "default_material";

    glm::vec3 m_ambientColor  = glm::vec3(0.1f);
    glm::vec3 m_diffuseColor  = glm::vec3(1.0f);
    glm::vec3 m_specularColor = glm::vec3(0.0f);
    float     m_shininess     = 32.0f;

    std::shared_ptr<Texture> m_diffuseTex;
    std::shared_ptr<Texture> m_specularTex;
    std::shared_ptr<Texture> m_emissionTex;
    std::shared_ptr<Texture> m_normalTex;

private:
    inline static GLuint s_lastTextureTargets[4] = { 0, 0, 0, 0 };
    inline static bool   s_hasUploadedData       = false;
};
