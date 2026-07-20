#include "Headers/Command/SetTechniqueCommand.h"
#include "Headers/Model/RayMarchedModel.h"

SetTechniqueCommand::SetTechniqueCommand(std::shared_ptr<RayMarchedModel> surface,
                                         std::shared_ptr<IRayMarchingTechnique> technique)
    : m_surface(std::move(surface)), m_technique(std::move(technique)) {}

void SetTechniqueCommand::Execute()
{
    m_surface->SetTechnique(m_technique);
}
