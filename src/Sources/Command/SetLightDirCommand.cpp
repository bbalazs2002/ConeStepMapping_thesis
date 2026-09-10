#include "Headers/Command/SetLightDirCommand.h"
#include "Headers/Manager/SceneManager.h"
#include "Headers/Model/RayMarchedModel.h"

SetLightDirCommand::SetLightDirCommand(SceneManager& scene, glm::vec3 lightDir)
    : m_sceneManager(scene), m_lightDir(lightDir) {}

void SetLightDirCommand::Execute()
{
    for (const auto& obj : m_sceneManager.GetSceneObjects()) {
        auto rm = std::dynamic_pointer_cast<RayMarchedModel>(obj);
        if (rm) rm->SetLightDir(m_lightDir);
    }
}
