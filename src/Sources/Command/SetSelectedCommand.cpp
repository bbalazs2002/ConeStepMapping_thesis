#include "Headers/Command/SetSelectedCommand.h"
#include "Headers/Manager/SceneManager.h"

SetSelectedCommand::SetSelectedCommand(SceneManager& sceneManager,
                                       std::shared_ptr<ISceneObject> target)
    : m_sceneManager(sceneManager), m_target(std::move(target))
{
}

void SetSelectedCommand::Execute()
{
    m_sceneManager.SetSelected(m_target.get());
}
