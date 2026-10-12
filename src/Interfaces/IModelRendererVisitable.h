// This file may contain AI-assisted (Claude Code) code, reviewed by the author.

#pragma once

interface IModelRendererVisitor;

interface IModelRendererVisitable {
public:
    virtual void AcceptRendererVisitor(IModelRendererVisitor& v) = 0;
    virtual ~IModelRendererVisitable() = default;
};
