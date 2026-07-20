#pragma once

#include <memory>
#include "Interfaces/ICommand.h"

class RayMarchedModel;

class SetEpsilonCommand : public ICommand {
public:
    SetEpsilonCommand(std::shared_ptr<RayMarchedModel> surface, float epsilon);
    void Execute() override;

private:
    std::shared_ptr<RayMarchedModel> m_surface;
    float                            m_epsilon;
};
