#pragma once

#include <memory>
#include "Interfaces/ICommand.h"

class Mesh;
class Material;

class SetMaterialCommand : public ICommand {
public:
    SetMaterialCommand(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material);
    void Execute() override;

private:
    std::shared_ptr<Mesh>     m_mesh;
    std::shared_ptr<Material> m_material;
};
