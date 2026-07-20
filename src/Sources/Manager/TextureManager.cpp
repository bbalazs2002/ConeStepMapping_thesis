#include "Headers/Manager/TextureManager.h"
#include "Headers/Texture/Texture.h"

std::shared_ptr<Texture> TextureManager::GetOrLoad(const std::filesystem::path& path, bool flip)
{
    CollectGarbage();
    std::string key = path.string();
    auto it = m_cache.find(key);
    if (it != m_cache.end()) {
        if (auto existing = it->second.lock())
            return existing;
    }
    auto tex = std::make_shared<Texture>(path, flip);
    m_cache[key] = tex;
    return tex;
}

std::shared_ptr<Texture> TextureManager::GetOrLoadCubemap(const std::array<std::filesystem::path, 6>& faces, bool flip)
{
    CollectGarbage();

    // Cache key: the 6 face paths joined together. Distinct from GetOrLoad()'s
    // single-path keys, so 2D textures and cubemaps never collide in m_cache.
    std::string key;
    for (const auto& face : faces)
        key += face.string() + "|";

    auto it = m_cache.find(key);
    if (it != m_cache.end()) {
        if (auto existing = it->second.lock())
            return existing;
    }
    auto tex = std::make_shared<Texture>(faces, flip);
    m_cache[key] = tex;
    return tex;
}

void TextureManager::CollectGarbage()
{
    for (auto it = m_cache.begin(); it != m_cache.end(); ) {
        if (it->second.expired())
            it = m_cache.erase(it);
        else
            ++it;
    }
}

size_t TextureManager::GetCachedCount() const
{
    size_t count = 0;
    for (const auto& [key, weak] : m_cache)
        if (!weak.expired()) ++count;
    return count;
}
