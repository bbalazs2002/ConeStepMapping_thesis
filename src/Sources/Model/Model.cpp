#include "Headers/Model/Model.h"
#include "Interfaces/IGUIVisitor.h"
#include "Interfaces/IModelRendererVisitor.h"

Model::Model(std::string name)
    : ModelBase(std::move(name))
{
}

void Model::AcceptGUIVisitor(IGUIVisitor& v)
{
    v.Visit(*this);
}

void Model::AcceptRendererVisitor(IModelRendererVisitor& v)
{
    v.Visit(*this);
}
