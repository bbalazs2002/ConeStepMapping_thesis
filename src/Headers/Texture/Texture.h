#pragma once

#include <array>
#include <filesystem>
#include <GL/glew.h>

class Texture {
public:
    explicit Texture(const std::filesystem::path& path, bool flip = false);

    // Creates an empty GPU texture (no pixel data). Intended for compute shader output,
    // where the shader writes directly via glBindImageTexture.
    Texture(int width, int height, GLenum internalFormat);

    // Creates a cubemap texture from 6 face images (+X,-X,+Y,-Y,+Z,-Z), e.g. for a skybox.
    explicit Texture(const std::array<std::filesystem::path, 6>& faces, bool flip = false);

    Texture(const Texture&)            = delete;
    Texture& operator=(const Texture&) = delete;

    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    ~Texture();

    inline GLuint GetID()   const { return m_id; }
    inline bool   IsValid() const { return m_id != 0; }
    inline GLenum GetType() const { return m_type; } // GL_TEXTURE_2D or GL_TEXTURE_CUBE_MAP

private:
    GLuint m_id   = 0;
    GLenum m_type = GL_TEXTURE_2D;
};
