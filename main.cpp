
#include <fmt/ranges.h>

#include <RewardMatrix.hpp>
#include <cassert>
#include <chrono>
#include <iostream>
#include <vector>

#include "Action.hpp"
#include "Player.hpp"
#include "RewardMatrix.hpp"
#define REPUTATION_STR "reputation"

using namespace std;

/**
 * @brief 主要执行体，可以并行
 *
 *
 * @param b
 * @param beta
 * @param c
 * @param gamma
 */
void func(double b, double beta, double c, double gamma, int population,
          int episode, int stepNum, double epsilon, double mu,
          double reputationP) {
  assert(b > 0 && beta > 0 && c > 0 && gamma > 0 &&
         "b, beta, c, gamma must be positive");

  RewardMatrix rewardMatrix("./rewardMatrix/RewardMatrix.csv");
  cout << rewardMatrix.getColNum() << endl;

  rewardMatrix.updateVar("b", b);
  rewardMatrix.updateVar("beta", beta);
  rewardMatrix.updateVar("c", c);
  rewardMatrix.updateVar("gamma", gamma);

  fmt::print("vars: {}\n", rewardMatrix.getVars());
  rewardMatrix.evalRewardMatrix();
  rewardMatrix.print();

  vector<Action> donorActions;
  donorActions.push_back(Action("C", 0));
  donorActions.push_back(Action("D", 1));
  Player temp_donor("donor", 0, donorActions, epsilon);
  temp_donor.initQTable({"0", "1"}, {"C", "D"});

  vector<Action> recipientActions;
  recipientActions.push_back(Action("C", 0));
  recipientActions.push_back(Action("D", 1));
  Player temp_recipient("recipient", 0, recipientActions, epsilon);
  temp_recipient.addVar(REPUTATION_STR, 0);
  temp_recipient.initQTable({"C", "D"}, {"C", "D"});

  // 初始化种群
  vector<Player> donors;
  vector<Player> recipients;
  for (int i = 0; i < population; i++) {
    Player donor(temp_donor);
    Player recipient(temp_recipient);

    donors.push_back(donor);
    if (recipient.getProbability() < reputationP) {
      recipient.updateVar(REPUTATION_STR, 1);
    } else {
      recipient.updateVar(REPUTATION_STR, 0);
    }

    recipients.push_back(recipient);
  }

  for (int i_episode = 0; i_episode < episode; i_episode++) {
    // TODO: episode 刷新buffer

    // 博弈过程
    for (int step = 0; step < stepNum; step++) {
      int di = donors[0].getRandomInt(0, population - 1);
      int ri = recipients[0].getRandomInt(0, population - 1);

      Action donorAction = donors[di].donate(
          to_string((int)recipients[di].getVarValue(REPUTATION_STR)), mu, true);
      Action recipientAction =
          recipients[di].reward(donorAction.getName(), mu, true);
      // cout << donorAction.getName() << endl;
      // cout << recipientAction.getName() << endl;
      double donorReward = 0;
      double recipientReward = 0;
      vector<double> rewards =
          rewardMatrix.getReward(donorAction, recipientAction);
      donorReward = rewards[0];
      recipientReward = rewards[1];
      // cout << donorReward << endl;
      // cout << recipientReward << endl;
      donors[di].updateScore(donorReward);
      recipients[di].updateScore(recipientReward);
      // cout << donors[di].getScore() << endl;
      // cout << recipients[di].getScore() << endl;

      // fmt::print("step: {}, di: {}, ri: {}, donorAction: {}, recipientAction:
      // {}, "
      //            "donorReward: {}, recipientReward: {}\n",
      //            step, di, ri, donorAction.getName(),
      //            recipientAction.getName(), donorReward, recipientReward);
    }
  }
}

int main() {
  // 记录运行时间
  chrono::system_clock::time_point start = chrono::system_clock::now();

  int b = 4;             // 公共参数
  int beta = 3;          // 公共参数
  int c = 1;             // 公共参数
  int gamma = 1;         // 公共参数
  int population = 100;  // 人口数量
  int stepNum = 200;   // 博弈轮数
  int episode = 20000;

  double epsilon = 0;        // epsilon-greedy
  double mu = 0;             // 动作突变率
  double reputationP = 0.5;  // 初始好声誉的概率

  func(b, beta, c, gamma, population, episode, stepNum, epsilon, mu, reputationP);

  chrono::system_clock::time_point end = chrono::system_clock::now();
  cout << "\ntime: "
       << chrono::duration_cast<chrono::microseconds>(end - start).count() / 1e6
       << "s" << endl;

  return 0;
}