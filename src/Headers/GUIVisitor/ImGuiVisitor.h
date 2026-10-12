// This file may contain AI-assisted (Claude Code) code, reviewed by the author.

#pragma once

#include "Interfaces/IGUIVisitor.h"

interface ICommandQueue;
class ModelBase;

class ImGuiVisitor : public IGUIVisitor {
public:
    explicit ImGuiVisitor(ICommandQueue& queue);

    void Visit(Model& target)           override;
    void Visit(RayMarchedModel& target) override;

private:
    void VisitModelBase(ModelBase& target);

    ICommandQueue& m_commandQueue;
};