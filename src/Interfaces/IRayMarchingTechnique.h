// This file may contain AI-assisted (Claude Code) code, reviewed by the author.

#pragma once

#include <string>
#include <GL/glew.h>

class RayMarchedModel;

interface IRayMarchingTechnique {
public:
    virtual GLuint      GetProgramID()                              const = 0;
    virtual void        SetUniforms(const RayMarchedModel& surface) const = 0;
    virtual std::string GetName()                                   const = 0;
    virtual int         GetTechniqueID()                            const = 0;
    virtual ~IRayMarchingTechnique() = default;
};