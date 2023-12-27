#include <gtest/gtest.h>
#include "PoolDeque.hpp"

TEST(PoolDequeTest, DefaultConstructor) {
    PoolDeque<int> deque;
    EXPECT_TRUE(deque.isEmpty());
    EXPECT_EQ(deque.size(), 0);
}

TEST(PoolDequeTest, ConstructorWithMaxSize) {
    PoolDeque<int> deque(10);
    EXPECT_TRUE(deque.isEmpty());
    EXPECT_EQ(deque.size(), 0);
}

TEST(PoolDequeTest, Enqueue) {
    PoolDeque<int> deque(10);
    deque.enQueue(1);
    EXPECT_FALSE(deque.isEmpty());
    EXPECT_EQ(deque.size(), 1);
    EXPECT_EQ(deque[0], 1);
}

TEST(PoolDequeTest, CopyConstructor) {
    PoolDeque<int> deque1(10);
    deque1.enQueue(1);
    PoolDeque<int> deque2(deque1);
    EXPECT_EQ(deque2.size(), 1);
    EXPECT_EQ(deque2[0], 1);
}

TEST(PoolDequeTest, AssignmentOperator) {
    PoolDeque<int> deque1(10);
    deque1.enQueue(1);
    PoolDeque<int> deque2;
    deque2 = deque1;
    EXPECT_EQ(deque2.size(), 1);
    EXPECT_EQ(deque2[0], 1);
}

TEST(PoolDequeTest, IsEmpty) {
    PoolDeque<int> deque(10);
    EXPECT_TRUE(deque.isEmpty());
    deque.enQueue(1);
    EXPECT_FALSE(deque.isEmpty());
}

TEST(PoolDequeTest, IsFull) {
    PoolDeque<int> deque(10);
    for (int i = 0; i < 10; i++) {
        deque.enQueue(i);
    }
    EXPECT_TRUE(deque.isFull());
}

// when the queue is full, the earliest element will be popped automatically
TEST(PoolDequeTest, AutoPop) {
    PoolDeque<int> deque(10);
    for (int i = 0; i < 20; i++) {
        deque.enQueue(i);
    }
    EXPECT_EQ(deque.size(), 10);
    EXPECT_EQ(deque[0], 10);
    EXPECT_EQ(deque[9], 19);
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}