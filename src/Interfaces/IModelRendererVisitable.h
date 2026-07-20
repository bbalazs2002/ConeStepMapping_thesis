#pragma once

interface IModelRendererVisitor;

interface IModelRendererVisitable {
public:
    virtual void AcceptRendererVisitor(IModelRendererVisitor& v) = 0;
    virtual ~IModelRendererVisitable() = default;
};
