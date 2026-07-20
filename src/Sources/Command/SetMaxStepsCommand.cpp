#include "Headers/Command/SetMaxStepsCommand.h"
#include "Headers/Model/RayMarchedModel.h"

SetMaxStepsCommand::SetMaxStepsCommand(std::shared_ptr<RayMarchedModel> surface, int maxSteps)
    : m_surface(std::move(surface)), m_maxSteps(maxSteps) {}

void SetMaxStepsCommand::Execute()
{
    m_surface->SetMaxSteps(m_maxSteps);
}
