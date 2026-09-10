#pragma once

#include <memory>
#include "Interfaces/ICommand.h"

class RayMarchedModel;

class SetNormalMultCommand : public ICommand {
public:
    SetNormalMultCommand(std::shared_ptr<RayMarchedModel> surface, float normalMult);
    void Execute() override;

private:
    std::shared_ptr<RayMarchedModel> m_surface;
    float                            m_normalMult;
};
