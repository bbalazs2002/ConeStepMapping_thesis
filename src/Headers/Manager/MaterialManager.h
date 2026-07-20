#pragma once

#include <unordered_map>
#include <memory>
#include <cstddef>

class Material;

class MaterialManager {
public:
    std::shared_ptr<Material> GetOrCreate(const Material& mat);

private:
    void CollectGarbage();
    std::unordered_map<std::size_t, std::weak_ptr<Material>> m_materials;
};
