#include "Headers/Command/SetRotationCommand.h"
#include "Headers/Model/ModelBase.h"

SetRotationCommand::SetRotationCommand(std::shared_ptr<ModelBase> target, const glm::quat& rotation)
    : m_target(std::move(target)), m_rotation(rotation) {}

void SetRotationCommand::Execute()
{
    m_target->GetTransform().SetRotation(m_rotation);
}
