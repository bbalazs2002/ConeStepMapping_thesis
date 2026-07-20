#include "Headers/Command/DeleteObjectCommand.h"
#include "Headers/Manager/SceneManager.h"

DeleteObjectCommand::DeleteObjectCommand(SceneManager& scene, std::shared_ptr<ISceneObject> target)
    : m_sceneManager(scene), m_target(std::move(target)) {}

void DeleteObjectCommand::Execute()
{
    m_sceneManager.Remove(m_target);
}