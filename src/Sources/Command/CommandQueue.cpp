#include <memory>

#include "Headers/Command/CommandQueue.h"
#include "Interfaces/ICommand.h"

void CommandQueue::Push(std::unique_ptr<ICommand> cmd)
{
    m_queue.push_back(std::move(cmd));
}

void CommandQueue::Execute()
{
    for (auto& cmd : m_queue)
        cmd->Execute();
    m_queue.clear();
}

void CommandQueue::Clear()
{
    m_queue.clear();
}
