#pragma once

#include <glm/glm.hpp>
#include "Interfaces/ICommand.h"

class SceneManager;

class SetLightDirCommand : public ICommand {
public:
    SetLightDirCommand(SceneManager& scene, glm::vec3 lightDir);
    void Execute() override;

private:
    SceneManager& m_sceneManager;
    glm::vec3     m_lightDir;
};
