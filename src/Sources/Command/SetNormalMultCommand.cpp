#include "Headers/Command/SetNormalMultCommand.h"
#include "Headers/Model/RayMarchedModel.h"

SetNormalMultCommand::SetNormalMultCommand(std::shared_ptr<RayMarchedModel> surface, float normalMult)
    : m_surface(std::move(surface)), m_normalMult(normalMult) {}

void SetNormalMultCommand::Execute()
{
    m_surface->SetNormalMult(m_normalMult);
}
