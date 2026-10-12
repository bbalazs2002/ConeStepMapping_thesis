// This file may contain AI-assisted (Claude Code) code, reviewed by the author.

#pragma once

interface ICommand {
public:
    virtual void Execute() = 0;
    virtual ~ICommand() = default;
};
