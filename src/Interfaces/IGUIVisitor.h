#pragma once

class Model;
class RayMarchedModel;

interface IGUIVisitor {
public:
    virtual void Visit(Model& target)          = 0;
    virtual void Visit(RayMarchedModel& target) = 0;
    virtual ~IGUIVisitor() = default;
};
