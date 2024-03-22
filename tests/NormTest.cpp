#include <gtest/gtest.h>
#include "Norm.hpp"
#include "Action.hpp"

TEST(NormTest, TestGetProbability) {
    Norm norm;
    double probability = norm.getProbability();
    EXPECT_GE(probability, 0.0);
    EXPECT_LE(probability, 1.0);
}

TEST(NormTest, TestGetReputation) {
    Norm norm("../norm/norm10.csv");
    Action donorAction("C", 0);
    Action recipientAction("D", 1);
    double reputation = norm.getReputation(donorAction, recipientAction, 0.0);
    EXPECT_EQ(reputation, 0.0);

    donorAction.setName("D");
    donorAction.setId(1);
    recipientAction.setName("C");
    recipientAction.setId(0);
    reputation = norm.getReputation(donorAction, recipientAction, 0.0);
    EXPECT_EQ(reputation, 1.0);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}