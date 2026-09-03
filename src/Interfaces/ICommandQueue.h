#pragma once

#include <memory>

interface ICommand;

interface ICommandQueue {
public:
    virtual void Push(std::unique_ptr<ICommand> cmd) = 0;
    virtual void Execute() = 0;
    virtual void Clear() = 0;
    virtual ~ICommandQueue() = default;
};
