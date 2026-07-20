#pragma once

#include <memory>
#include "Interfaces/ICommand.h"

class Model;
class SceneManager;

class CreateObjObjectCommand : public ICommand {
public:
    CreateObjObjectCommand(SceneManager& scene, std::shared_ptr<Model> model);
    void Execute() override;

private:
    SceneManager&          m_sceneManager;
    std::shared_ptr<Model> m_model;
};
