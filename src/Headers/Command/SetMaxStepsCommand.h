#pragma once

#include <memory>
#include "Interfaces/ICommand.h"

class RayMarchedModel;

class SetMaxStepsCommand : public ICommand {
public:
    SetMaxStepsCommand(std::shared_ptr<RayMarchedModel> surface, int maxSteps);
    void Execute() override;

private:
    std::shared_ptr<RayMarchedModel> m_surface;
    int                              m_maxSteps;
};
