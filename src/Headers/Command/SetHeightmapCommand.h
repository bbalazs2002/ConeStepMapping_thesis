#pragma once

#include <memory>
#include <string>
#include "Interfaces/ICommand.h"

class RayMarchedModel;
class ConemapGenerator;
class TextureManager;

class SetHeightmapCommand : public ICommand {
public:
    SetHeightmapCommand(std::shared_ptr<RayMarchedModel> surface,
                        TextureManager& texManager,
                        std::string path,
                        ConemapGenerator* gen = nullptr);
    void Execute() override;

private:
    std::shared_ptr<RayMarchedModel> m_surface;
    TextureManager&                  m_textureManager;
    std::string                      m_path;
    ConemapGenerator*                m_generator;
};
