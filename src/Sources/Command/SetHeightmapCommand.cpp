#include "Headers/Command/SetHeightmapCommand.h"
#include "Headers/Model/RayMarchedModel.h"
#include "Headers/Manager/TextureManager.h"

SetHeightmapCommand::SetHeightmapCommand(std::shared_ptr<RayMarchedModel> surface,
                                         TextureManager& texManager,
                                         std::string path,
                                         ConemapGenerator* gen)
    : m_surface(std::move(surface))
    , m_textureManager(texManager)
    , m_path(std::move(path))
    , m_generator(gen) {}

void SetHeightmapCommand::Execute()
{
    auto texture = m_textureManager.GetOrLoad(m_path);
    m_surface->SetHeightmap(std::move(texture), m_generator);
}
