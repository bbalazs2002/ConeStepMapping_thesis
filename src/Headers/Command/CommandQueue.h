#pragma once

#include <vector>
#include <memory>
#include "Interfaces/ICommandQueue.h"

class CommandQueue : public ICommandQueue {
public:
    void Push(std::unique_ptr<ICommand> cmd) override;
    void Execute() override;
    void Clear() override;

private:
    std::vector<std::unique_ptr<ICommand>> m_queue;
};
