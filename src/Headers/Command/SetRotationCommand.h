#pragma once

#include <memory>
#include <glm/gtc/quaternion.hpp>
#include "Interfaces/ICommand.h"

class ModelBase;

class SetRotationCommand : public ICommand {
public:
    SetRotationCommand(std::shared_ptr<ModelBase> target, const glm::quat& rotation);
    void Execute() override;

private:
    std::shared_ptr<ModelBase> m_target;
    glm::quat                  m_rotation;
};
