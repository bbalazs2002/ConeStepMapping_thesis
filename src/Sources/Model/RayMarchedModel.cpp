#include "Headers/Model/RayMarchedModel.h"
#include "Headers/RayMarching/ConemapGenerator.h"
#include "Headers/Texture/Texture.h"
#include "Interfaces/IGUIVisitor.h"
#include "Interfaces/IModelRendererVisitor.h"
#include "Utils/Log.h"

RayMarchedModel::RayMarchedModel(std::string name)
    : ModelBase(std::move(name))
{
}

GLuint RayMarchedModel::GetProgramID() const
{
    if (!m_technique) {
        LOG_ERROR("RayMarchedModel '", m_name, "': GetProgramID called but no technique is set");
        return 0;
    }
    return m_technique->GetProgramID();
}

void RayMarchedModel::SetTechnique(std::shared_ptr<IRayMarchingTechnique> t)
{
    m_technique = std::move(t);
}

void RayMarchedModel::SetHeightmap(std::shared_ptr<Texture> heightmap, ConemapGenerator* gen)
{
    if (gen == nullptr) {
        LOG_ERROR("RayMarchedModel '", m_name, "': SetHeightmap called with gen == nullptr — conemap not generated");
        return;
    }
    m_conemap = gen->Generate(*heightmap);
}

void RayMarchedModel::AcceptGUIVisitor(IGUIVisitor& v)
{
    v.Visit(*this);
}

void RayMarchedModel::AcceptRendererVisitor(IModelRendererVisitor& v)
{
    v.Visit(*this);
}
