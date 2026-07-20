#include <gtest/gtest.h>
#include "Headers/Command/CommandQueue.h"
#include "Interfaces/ICommand.h"
#include <vector>

// Minimal mock command — records execution in a shared counter
class CountCommand : public ICommand {
public:
    int& counter;
    int  increment;
    CountCommand(int& c, int inc = 1) : counter(c), increment(inc) {}
    void Execute() override { counter += increment; }
};

// Records execution order by index
class OrderCommand : public ICommand {
public:
    std::vector<int>& log;
    int id;
    OrderCommand(std::vector<int>& l, int i) : log(l), id(i) {}
    void Execute() override { log.push_back(id); }
};

// UT-14: FIFO execution order
TEST(CommandQueue, ExecutesInInsertionOrder) {
    CommandQueue q;
    std::vector<int> order;
    q.Push(std::make_unique<OrderCommand>(order, 1));
    q.Push(std::make_unique<OrderCommand>(order, 2));
    q.Push(std::make_unique<OrderCommand>(order, 3));
    q.Execute();

    ASSERT_EQ(order.size(), 3u);
    EXPECT_EQ(order[0], 1);
    EXPECT_EQ(order[1], 2);
    EXPECT_EQ(order[2], 3);
}

// UT-15: queue is empty after Execute — second Execute is a no-op
TEST(CommandQueue, EmptyAfterExecute) {
    CommandQueue q;
    int count = 0;
    q.Push(std::make_unique<CountCommand>(count));
    q.Execute();
    EXPECT_EQ(count, 1);
    q.Execute();
    EXPECT_EQ(count, 1); // no additional increments
}

// UT-16: Clear drops commands without executing them
TEST(CommandQueue, ClearPreventsExecution) {
    CommandQueue q;
    int count = 0;
    q.Push(std::make_unique<CountCommand>(count));
    q.Push(std::make_unique<CountCommand>(count));
    q.Clear();
    q.Execute();
    EXPECT_EQ(count, 0);
}

// Multiple pushes accumulate before Execute
TEST(CommandQueue, MultipleCommandsAllExecute) {
    CommandQueue q;
    int count = 0;
    for (int i = 0; i < 10; ++i)
        q.Push(std::make_unique<CountCommand>(count));
    q.Execute();
    EXPECT_EQ(count, 10);
}

// Push → Execute → Push → Execute: second batch runs independently
TEST(CommandQueue, TwoBatchesRunIndependently) {
    CommandQueue q;
    int count = 0;
    q.Push(std::make_unique<CountCommand>(count, 5));
    q.Execute();
    EXPECT_EQ(count, 5);

    q.Push(std::make_unique<CountCommand>(count, 3));
    q.Execute();
    EXPECT_EQ(count, 8);
}

// Empty Execute is safe (no crash)
TEST(CommandQueue, EmptyExecuteIsNoop) {
    CommandQueue q;
    EXPECT_NO_THROW(q.Execute());
}

// Clear on empty queue is safe
TEST(CommandQueue, ClearOnEmptyIsNoop) {
    CommandQueue q;
    EXPECT_NO_THROW(q.Clear());
}
