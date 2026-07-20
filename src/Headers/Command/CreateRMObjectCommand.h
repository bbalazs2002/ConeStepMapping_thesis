#pragma once

#include <memory>
#include "Interfaces/ICommand.h"

class RayMarchedModel;
class SceneManager;

class CreateRMObjectCommand : public ICommand {
public:
    CreateRMObjectCommand(SceneManager& scene, std::shared_ptr<RayMarchedModel> model);
    void Execute() override;

private:
    SceneManager&                    m_sceneManager;
    std::shared_ptr<RayMarchedModel> m_model;
};
