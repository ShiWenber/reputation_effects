#include <gtest/gtest.h>
#include "MyRandom.hpp"
#include <fstream>
#include <vector>
#include <tbb/task_arena.h>
#include <tbb/parallel_for.h>

TEST(MyRandomTest, TestGetProbability) {
    unsigned seed = 456;
    MyRandom& random = MyRandom::getInstance(seed);
    // 结果写入文件
    std::ofstream outfile;
    outfile.open("probability.txt", std::ios::app);
    for (int i = 0; i < 1000; ++i) {
        double probability = random.getProbability();
        // Check that the probability is in the range [0, 1]
        outfile << probability << std::endl;
        EXPECT_GE(probability, 0.0);
        EXPECT_LE(probability, 1.0);
    }
    outfile.close();
}

// 验证单例模式
TEST(MyRandomTest, TestGetProbability2) {
    unsigned seed = 123;
    MyRandom& random1 = MyRandom::getInstance(seed);
    MyRandom& random2 = MyRandom::getInstance(456);
    // Check that the two instances are the same
    EXPECT_EQ(&random1, &random2);
    EXPECT_EQ(random1.getCurrentSeed(), random2.getCurrentSeed());
}

TEST(MyRandomTest, TestGetProbability3) {
    EXPECT_EQ(MyRandom::getInstance().getCurrentSeed(), 456);
}

// 线程内局部单例
TEST(MyRandomTest, TestGetProbability4) {
    std::vector<unsigned> seeds = {123, 456, 123, 101112};
    std::vector<std::vector<double>> res;
    for (int i = 0; i < 4; ++i) {
        MyRandom& random = MyRandom::getInstance(seeds[i]);
        std::vector<double> tmp(1000);
        res.push_back(tmp);
    }
    
    // tbb::task_arena arena(4);
    // arena.execute([&] {
    //     // tbb::parallel_for(0, 4, [&](int i) {
    //     //     MyRandom& random = MyRandom::getInstance(seeds[i]);
    //     //     EXPECT_EQ(random.getCurrentSeed(), seeds[i]);
    //     //     for (int j = 0; j < 1000; ++j) {
    //     //         res[i][j] = random.getProbability();
    //     //     }
    //     // });
    // });

    std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i) {
      threads.emplace_back([&, i]() {
        MyRandom& random = MyRandom::getInstance(seeds[i]);
        EXPECT_EQ(random.getCurrentSeed(), seeds[i]);
        for (int j = 0; j < 1000; ++j) {
          res[i][j] = random.getProbability();
        }
      });
    }

    // Join all threads
    for (auto& thread : threads) {
      thread.join();
    }

    for (int j = 0; j < 1000; ++j) {
        EXPECT_EQ(res[0][j], res[2][j]);
    }
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}