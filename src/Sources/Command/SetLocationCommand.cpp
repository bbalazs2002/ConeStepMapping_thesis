#include "Headers/Command/SetLocationCommand.h"
#include "Headers/Model/ModelBase.h"

SetLocationCommand::SetLocationCommand(std::shared_ptr<ModelBase> target, const glm::vec3& location)
    : m_target(std::move(target)), m_location(location) {}

void SetLocationCommand::Execute()
{
    m_target->GetTransform().SetLocation(m_location);
}
