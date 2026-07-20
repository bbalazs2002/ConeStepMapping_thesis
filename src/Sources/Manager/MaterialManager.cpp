#include "Headers/Manager/MaterialManager.h"
#include "Headers/Material/Material.h"

std::shared_ptr<Material> MaterialManager::GetOrCreate(const Material& mat)
{
    CollectGarbage();
    std::size_t hash = Material::Hash(mat);
    auto it = m_materials.find(hash);
    if (it != m_materials.end()) {
        if (auto existing = it->second.lock())
            return existing;
    }
    auto newMat = std::make_shared<Material>(mat);
    m_materials[hash] = newMat;
    return newMat;
}

void MaterialManager::CollectGarbage()
{
    for (auto it = m_materials.begin(); it != m_materials.end(); ) {
        if (it->second.expired())
            it = m_materials.erase(it);
        else
            ++it;
    }
}
