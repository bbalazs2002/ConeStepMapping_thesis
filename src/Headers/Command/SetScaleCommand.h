#pragma once

#include <memory>
#include <glm/glm.hpp>
#include "Interfaces/ICommand.h"

class ModelBase;

class SetScaleCommand : public ICommand {
public:
    SetScaleCommand(std::shared_ptr<ModelBase> target, const glm::vec3& scale);
    void Execute() override;

private:
    std::shared_ptr<ModelBase> m_target;
    glm::vec3                  m_scale;
};
