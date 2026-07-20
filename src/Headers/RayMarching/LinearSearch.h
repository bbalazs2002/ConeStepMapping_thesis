#pragma once

#include <string>
#include <GL/glew.h>

#include "Interfaces/IRayMarchingTechnique.h"

class RayMarchedModel;

class LinearSearch : public IRayMarchingTechnique {
public:
    explicit LinearSearch(GLuint programID);

    GLuint      GetProgramID()                              const override;
    void        SetUniforms(const RayMarchedModel& surface) const override;
    std::string GetName()                                   const override;
    int         GetTechniqueID()                            const override { return 0; }

    void SetProgramID(GLuint id) { m_programID = id; }

private:
    GLuint m_programID;
};
