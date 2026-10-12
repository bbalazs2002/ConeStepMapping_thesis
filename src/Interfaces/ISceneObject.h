// This file may contain AI-assisted (Claude Code) code, reviewed by the author.

#pragma once

#include <string>
#include "Interfaces/IGUIVisitable.h"

interface ISceneObject : public IGUIVisitable {
public:
    virtual const std::string& GetName() const = 0;
    virtual ~ISceneObject() = default;
};
