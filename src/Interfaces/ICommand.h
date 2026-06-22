#pragma once

interface ICommand {
public:
    virtual ~ICommand() = default;
    virtual void Execute() = 0;
};
