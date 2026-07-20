#include "Headers/Command/CreateRMObjectCommand.h"
#include "Headers/Manager/SceneManager.h"
#include "Headers/Model/RayMarchedModel.h"

CreateRMObjectCommand::CreateRMObjectCommand(SceneManager& scene, std::shared_ptr<RayMarchedModel> model)
    : m_sceneManager(scene), m_model(std::move(model)) {}

void CreateRMObjectCommand::Execute()
{
    if (m_model)
        m_sceneManager.Add(std::move(m_model));
}
