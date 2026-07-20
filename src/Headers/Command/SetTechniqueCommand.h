#pragma once

#include <memory>
#include "Interfaces/ICommand.h"

class RayMarchedModel;
interface IRayMarchingTechnique;

class SetTechniqueCommand : public ICommand {
public:
    SetTechniqueCommand(std::shared_ptr<RayMarchedModel> surface,
                        std::shared_ptr<IRayMarchingTechnique> technique);
    void Execute() override;

private:
    std::shared_ptr<RayMarchedModel>       m_surface;
    std::shared_ptr<IRayMarchingTechnique> m_technique;
};
