#pragma once

#include "Types.h"

interface IUpdatable {
public:
    virtual void Update(const SUpdateInfo& info) = 0;
    virtual ~IUpdatable() = default;
};
