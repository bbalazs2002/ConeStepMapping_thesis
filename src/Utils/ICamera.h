// This file may contain AI-assisted (Claude Code) code, reviewed by the author.

#pragma once

#include <glm/glm.hpp>

interface ICamera {
public:
    virtual glm::mat4 GetViewProj() const = 0;
    virtual glm::vec3 GetEye()      const = 0;
    virtual glm::vec3 GetAt()       const = 0;
    virtual void      SetAspect(float aspect) = 0;
    virtual ~ICamera() = default;
};
