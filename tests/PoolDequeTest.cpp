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

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}