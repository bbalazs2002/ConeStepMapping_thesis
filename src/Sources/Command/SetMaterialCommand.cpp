#include "Headers/Command/SetMaterialCommand.h"
#include "Headers/Model/Mesh.h"

SetMaterialCommand::SetMaterialCommand(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material)
    : m_mesh(std::move(mesh)), m_material(std::move(material)) {}

void SetMaterialCommand::Execute()
{
    m_mesh->SetMaterial(m_material);
}
