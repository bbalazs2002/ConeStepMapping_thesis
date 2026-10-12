// This file may contain AI-assisted (Claude Code) code, reviewed by the author.

#pragma once

#include <memory>
#include "Interfaces/ICommand.h"

class SceneManager;
interface ISceneObject;

class SetSelectedCommand : public ICommand {
public:
    SetSelectedCommand(SceneManager& sceneManager, std::shared_ptr<ISceneObject> target);
    void Execute() override;

private:
    SceneManager&                  m_sceneManager;
    std::shared_ptr<ISceneObject>  m_target;
};
