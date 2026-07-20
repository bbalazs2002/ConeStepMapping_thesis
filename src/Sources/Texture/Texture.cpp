#include "Texture/Texture.h"

#include <algorithm>
#include <cmath>

#include "GLUtils.hpp"
#include "Log.h"

Texture::Texture(const std::filesystem::path& path, bool flip)
    : m_type(GL_TEXTURE_2D)
{
    ImageRGBA img = ImageFromFile(path, flip);
    if (img.width <= 0 || img.height <= 0) {
        LOG_ERROR("[Texture] Failed to load: ", path.generic_string());
        return;
    }

    glCreateTextures(GL_TEXTURE_2D, 1, &m_id);

    int levels = 1 + static_cast<int>(
        std::floor(std::log2(static_cast<float>(std::max(img.width, img.height)))));

    glTextureStorage2D(m_id, levels, GL_RGBA8, img.width, img.height);
    glTextureSubImage2D(m_id, 0, 0, 0, img.width, img.height,
                        GL_RGBA, GL_UNSIGNED_BYTE, img.data());
    glGenerateTextureMipmap(m_id);

    glTextureParameteri(m_id, GL_TEXTURE_WRAP_S,     GL_REPEAT);
    glTextureParameteri(m_id, GL_TEXTURE_WRAP_T,     GL_REPEAT);
    glTextureParameteri(m_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(m_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

Texture::Texture(int width, int height, GLenum internalFormat)
    : m_type(GL_TEXTURE_2D)
{
    glCreateTextures(GL_TEXTURE_2D, 1, &m_id);
    glTextureStorage2D(m_id, 1, internalFormat, width, height);

    glTextureParameteri(m_id, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_id, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(m_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

Texture::Texture(const std::array<std::filesystem::path, 6>& faces, bool flip)
    : m_type(GL_TEXTURE_CUBE_MAP)
{
    ImageRGBA images[6];
    for (int i = 0; i < 6; ++i) {
        images[i] = ImageFromFile(faces[i], flip);
        if (images[i].width <= 0 || images[i].height <= 0) {
            LOG_ERROR("[Texture] Failed to load cubemap face: ", faces[i].generic_string());
            return;
        }
    }

    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_id);
    glTextureStorage2D(m_id, 1, GL_RGBA8, images[0].width, images[0].height);
    for (int face = 0; face < 6; ++face) {
        glTextureSubImage3D(m_id, 0, 0, 0, face,
            images[face].width, images[face].height, 1,
            GL_RGBA, GL_UNSIGNED_BYTE, images[face].data());
    }

    glTextureParameteri(m_id, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_id, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_id, GL_TEXTURE_WRAP_R,     GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(m_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}

Texture::Texture(Texture&& other) noexcept
    : m_id(other.m_id), m_type(other.m_type)
{
    other.m_id = 0;
}

Texture& Texture::operator=(Texture&& other) noexcept
{
    if (this != &other) {
        if (m_id != 0) glDeleteTextures(1, &m_id);
        m_id = other.m_id;
        m_type = other.m_type;
        other.m_id = 0;
    }
    return *this;
}

Texture::~Texture()
{
    if (m_id != 0) glDeleteTextures(1, &m_id);
}
