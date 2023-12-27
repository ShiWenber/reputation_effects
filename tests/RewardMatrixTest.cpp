#include "gtest/gtest.h"
#include "RewardMatrix.hpp"
#include "Action.hpp"

TEST(RewardMatrixTest, ConstructorTest) {
    // create a RewardMatrix object with a valid csv file path
    RewardMatrix rm("../rewardMatrix/RewardMatrix.csv");

    // check if the row number, column number and player number are correct
    EXPECT_EQ(rm.getRowNum(), 2);
    EXPECT_EQ(rm.getColNum(), 2);
    EXPECT_EQ(rm.getPlayerNum(), 2);
}

TEST(RewardMatrixTest, GetRewardTest) {
    int b = 4;
    int beta = 3;
    int c = 1;
    int gamma = 1;
    int p0 = 1;

    RewardMatrix rm("../rewardMatrix/RewardMatrix.csv");
    rm.updateVar("b", b);
    rm.updateVar("beta", beta);
    rm.updateVar("c", c);
    rm.updateVar("gamma", gamma);
    rm.updateVar("p", p0);
    rm.evalRewardMatrix();

    auto result = rm.getReward(Action("D", 1), Action("D", 1));
    EXPECT_EQ(result[0], 0); //< donor reward
    EXPECT_EQ(result[1], 0); //< recipient reward

    result = rm.getReward(Action("D", 1), Action("C", 0));
    EXPECT_EQ(result[0], beta); //< donor reward
    EXPECT_EQ(result[1], -gamma); //< recipient reward
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}