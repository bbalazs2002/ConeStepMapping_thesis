#include "Headers/Command/SetEpsilonCommand.h"
#include "Headers/Model/RayMarchedModel.h"

SetEpsilonCommand::SetEpsilonCommand(std::shared_ptr<RayMarchedModel> surface, float epsilon)
    : m_surface(std::move(surface)), m_epsilon(epsilon) {}

void SetEpsilonCommand::Execute()
{
    m_surface->SetEpsilon(m_epsilon);
}
