#include "Headers/Manager/TextureManager.h"
#include "Headers/Texture/Texture.h"

std::shared_ptr<Texture> TextureManager::GetOrLoad(const std::filesystem::path& path, bool flip)
{
    CollectGarbage();
    // flip is part of the key: same file loaded with different flip produces a
    // different GL texture, so they must not share a cache entry.
    std::string key = path.string() + (flip ? "|1" : "|0");
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

    std::string key;
    for (const auto& face : faces)
        key += face.string() + "|";
    key += (flip ? "1" : "0");

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
