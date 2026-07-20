#pragma once

#include <string>
#include "Interfaces/IGUIVisitable.h"

interface ISceneObject : public IGUIVisitable {
public:
    virtual const std::string& GetName() const = 0;
    virtual ~ISceneObject() = default;
};
