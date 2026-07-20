#include "Headers/RayMarching/ConeStepMapping.h"
#include "Headers/Model/RayMarchedModel.h"
#include "Headers/Texture/Texture.h"
#include "Utils/Log.h"

#include <glm/gtc/type_ptr.hpp>

ConeStepMapping::ConeStepMapping(GLuint programID)
    : m_programID(programID)
{
}

GLuint ConeStepMapping::GetProgramID() const
{
    return m_programID;
}

std::string ConeStepMapping::GetName() const
{
    return "Cone Step Mapping";
}

void ConeStepMapping::SetUniforms(const RayMarchedModel& surface) const
{
    if (auto cm = surface.GetConemap()) {
        glBindTextureUnit(4, cm->GetID());
    } else {
        LOG_ERROR("ConeStepMapping::SetUniforms: conemap is null on '", surface.GetName(), "' — run ConemapGenerator first");
    }

    glUniform1i (glGetUniformLocation(m_programID, "maxSteps"),            surface.GetMaxSteps());
    glUniform1f (glGetUniformLocation(m_programID, "epsilon"),             surface.GetEpsilon());
    glUniform1f (glGetUniformLocation(m_programID, "normalMult"),          surface.GetNormalMult());
    glUniform1i (glGetUniformLocation(m_programID, "discardFragments"),    surface.GetDiscardFragments()    ? 1 : 0);
    glUniform1i (glGetUniformLocation(m_programID, "displayNonConverged"), surface.GetDisplayNonConverged() ? 1 : 0);

    glUniform3fv(glGetUniformLocation(m_programID, "lightDir"), 1, glm::value_ptr(surface.GetLightDir()));

    // showFlags: per-model FS flag-colour visualisation (SSBO debug is handled separately)
    glUniform1i(glGetUniformLocation(m_programID, "dbg.showFlags"), surface.GetShowFlags() ? 1 : 0);
}
