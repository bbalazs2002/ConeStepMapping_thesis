// This file may contain AI-assisted (Claude Code) code, reviewed by the author.

#include "Headers/Model/ModelBase.h"

ModelBase::ModelBase(std::string name)
    : m_name(std::move(name))
{
}

void ModelBase::Update(const SUpdateInfo& /*info*/)
{
}
