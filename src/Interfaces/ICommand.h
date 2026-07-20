#pragma once

interface ICommand {
public:
    virtual void Execute() = 0;
    virtual ~ICommand() = default;
};
