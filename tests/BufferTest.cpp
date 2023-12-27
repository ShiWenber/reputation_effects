#include <gtest/gtest.h>
#include "Buffer.hpp"
#include "Transition.hpp"
#include <vector>
#include <iostream>

TEST(BufferTest, AddTransitionTest) {
    Buffer buffer(2, 10, 2);
    Transition transition("state1", Action("D", 1), 3.0, "state2");
    buffer.add(0, 6, transition);
    std::vector<Transition> result = buffer.sample(0, 6, 1);
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].getState(), "state1");
    EXPECT_EQ(result[0].getReward(), 3.0);
    EXPECT_EQ(result[0].getNextState(), "state2");
    EXPECT_EQ(result[0].getAction(), Action("D", 1));
}

TEST(BufferTest, MaxSizeTest) {
    Buffer buffer(2, 10, 2);
    Transition transition("state1", Action(), 3.0, "state2");
    Transition transition2("state1", Action(), 4.0, "state2");
    buffer.add(0, 6, transition);
    buffer.add(0, 6, transition);
    std::vector<Transition> result = buffer.sample(0, 6, 1);
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].getState(), "state1");
    EXPECT_EQ(result[0].getReward(), 3.0);
    EXPECT_EQ(result[0].getNextState(), "state2");
    buffer.add(0, 6, transition2);
    buffer.add(0, 6, transition2);
    result = buffer.sample(0, 6, 1);
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].getState(), "state1");
    EXPECT_EQ(result[0].getReward(), 4.0);
    EXPECT_EQ(result[0].getNextState(), "state2");
}

TEST(BufferTest, SampleTest) {
    Buffer buffer(2, 10, 2);
    Transition transition("state1", Action(), 3.0, "state2");
    buffer.add(0, 6, transition);
    std::vector<Transition> result = buffer.sample(0, 6, 1);
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].getState(), "state1");
    EXPECT_EQ(result[0].getReward(), 3.0);
    EXPECT_EQ(result[0].getNextState(), "state2");
}

TEST(TransitionTest, ConstructorTest) {
    Transition transition("state1", Action(), 3.0, "state2");
    EXPECT_EQ(transition.getState(), "state1");
    EXPECT_EQ(transition.getReward(), 3.0);
    EXPECT_EQ(transition.getNextState(), "state2");
}

TEST(TransitionTest, CopyConstructorTest) {
    Transition transition("state1", Action(), 3.0, "state2");
    Transition transition2(transition);
    EXPECT_EQ(transition2.getState(), "state1");
    EXPECT_EQ(transition2.getReward(), 3.0);
    EXPECT_EQ(transition2.getNextState(), "state2");
}


// 插入 6 个 transition 到 buffer 中，然后再取出 6 个 transition，看是否一致
// 并 cout 3 个随机结果
TEST(TransitionTest, SampleTest) {
    std::vector<Transition> batch;
    for (int i = 0; i < 6; i++)
    {
        batch.push_back(Transition("state1", Action(), i, "state2"));
    }

    Buffer buffer(1, 1, 6);
    for (auto const& transition : batch)
    {
        buffer.add(0, 0, transition);
    }

    // sample a batch of 6 transition 100 times
    for (int i = 0; i < 100; i++)
    {   
        batch = buffer.sample(0, 0, 6);
        EXPECT_EQ(batch.size(), 6); 
        // 保证batch内所有的transition都是不同的
        for (int j = 0; j < 6; j++)
        {
            for (int k = j + 1; k < 6; k++)
            {
                EXPECT_NE(batch[j].getReward(), batch[k].getReward());
            }
        }
    }
}


int main(int argc, char **argv) {
    Transition transition("state1", Action(), 3.0, "state2");
    Transition transition2(transition);
    Transition transition3(transition);
    Transition transition4("0", Action(), 3.5, "state2");
    Transition transition5(transition4);
    Transition transition6(transition4);

    Buffer buffer(1, 1, 6);
    buffer.add(0, 0, transition);
    buffer.add(0, 0, transition2);
    buffer.add(0, 0, transition3);
    buffer.add(0, 0, transition4);
    buffer.add(0, 0, transition5);
    buffer.add(0, 0, transition6);

    std::vector<Transition> batch;

    for (int i = 0; i < 6; i++)
    {   
        batch = buffer.sample(0, 0, 2);
        EXPECT_EQ(batch.size(), 2); 
        std::cout << batch[0].getState() << batch[1].getState() << std::endl;
    }
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}