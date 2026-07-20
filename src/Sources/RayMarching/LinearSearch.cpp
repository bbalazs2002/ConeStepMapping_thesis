#include "Headers/RayMarching/LinearSearch.h"
#include "Headers/Model/RayMarchedModel.h"
#include "Headers/Texture/Texture.h"

#include <glm/gtc/type_ptr.hpp>

LinearSearch::LinearSearch(GLuint programID)
    : m_programID(programID)
{
}

GLuint LinearSearch::GetProgramID() const
{
    return m_programID;
}

std::string LinearSearch::GetName() const
{
    return "Linear Search";
}

void LinearSearch::SetUniforms(const RayMarchedModel& surface) const
{
    if (auto cm = surface.GetConemap())
        glBindTextureUnit(4, cm->GetID());

    glUniform1i (glGetUniformLocation(m_programID, "maxSteps"),            surface.GetMaxSteps());
    glUniform1f (glGetUniformLocation(m_programID, "normalMult"),          surface.GetNormalMult());
    glUniform1i (glGetUniformLocation(m_programID, "discardFragments"),    surface.GetDiscardFragments()    ? 1 : 0);
    glUniform1i (glGetUniformLocation(m_programID, "displayNonConverged"), surface.GetDisplayNonConverged() ? 1 : 0);

    glUniform3fv(glGetUniformLocation(m_programID, "lightDir"), 1, glm::value_ptr(surface.GetLightDir()));
}
