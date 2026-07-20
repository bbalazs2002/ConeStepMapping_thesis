#pragma once

#include <array>
#include <unordered_map>
#include <memory>
#include <filesystem>
#include <string>

class Texture;

class TextureManager {
public:
    std::shared_ptr<Texture> GetOrLoad(const std::filesystem::path& path, bool flip = false);
    std::shared_ptr<Texture> GetOrLoadCubemap(const std::array<std::filesystem::path, 6>& faces, bool flip = false);
    size_t GetCachedCount() const;

private:
    void CollectGarbage();
    std::unordered_map<std::string, std::weak_ptr<Texture>> m_cache;
};
