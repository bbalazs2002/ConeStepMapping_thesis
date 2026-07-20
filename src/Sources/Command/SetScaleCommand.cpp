#include "Headers/Command/SetScaleCommand.h"
#include "Headers/Model/ModelBase.h"

SetScaleCommand::SetScaleCommand(std::shared_ptr<ModelBase> target, const glm::vec3& scale)
    : m_target(std::move(target)), m_scale(scale) {}

void SetScaleCommand::Execute()
{
    m_target->GetTransform().SetScale(m_scale);
}
