#include "Headers/Command/CreateObjObjectCommand.h"
#include "Headers/Manager/SceneManager.h"
#include "Headers/Model/Model.h"

CreateObjObjectCommand::CreateObjObjectCommand(SceneManager& scene, std::shared_ptr<Model> model)
    : m_sceneManager(scene), m_model(std::move(model)) {}

void CreateObjObjectCommand::Execute()
{
    if (m_model)
        m_sceneManager.Add(std::move(m_model));
}
