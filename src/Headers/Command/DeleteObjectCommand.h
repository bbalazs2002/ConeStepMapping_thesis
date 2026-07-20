#pragma once

#include <memory>
#include "Interfaces/ICommand.h"

interface ISceneObject;
class SceneManager;

class DeleteObjectCommand : public ICommand {
public:
    DeleteObjectCommand(SceneManager& scene, std::shared_ptr<ISceneObject> target);
    void Execute() override;

private:
    SceneManager&                m_sceneManager;
    std::shared_ptr<ISceneObject> m_target;
};