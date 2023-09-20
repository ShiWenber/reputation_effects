/**
 * @file rock-paper-scissors.cpp
 * @author ShiWenber (1210169842@qq.com)
 * @brief 石头剪刀布博弈模拟
 * @version 0.1
 * @date 2023-09-12
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <iostream>
// 导入字典类型
#include <fmt/ranges.h>

#include <filesystem>
#include <fstream>
#include <map>
#include <string>

#include "Action.hpp"
#include "Norm.hpp"
#include "PayoffMatrix.hpp"
#include "Player.hpp"
#include "Strategy.hpp"

#define REPUTATION_STR "reputation"

using namespace std;

/**
 * @brief 费米函数，用来计算策略转变概率
 * 
 * @param payoff1 
 * @param payoff2 
 * @param s 
 * @return double 
 */
double fermi(double payoff1, double payoff2, double s) {
  return 1 / (1 + exp((payoff2 - payoff1) / s));
}

int main() {
  // 博弈参数
  int stepNum = 10000;
  cout << "---" << endl;

  // 加载norm
  Norm norm("./norm.csv");
  fmt::print("norm: {}\n", norm.getNormTableStr());

  // 加载payoffMatrix
  cout << "---------->>" << endl;
  PayoffMatrix payoffMatrix_g("./GPayoffMatrix.csv");
  fmt::print("payoffMatrix_g: {}\n", payoffMatrix_g.getPayoffMatrixStr());
  // 输出需要赋值的所有变量
  fmt::print("vars: {}\n", payoffMatrix_g.getVars());
  cout << "after assign:" << endl;

  // vars: {b: 0, beta: 0, c: 0, gamma: 0, lambda: 0, }
  // 赋值后
  payoffMatrix_g.updateVar("b", 4);
  payoffMatrix_g.updateVar("beta", 3);
  payoffMatrix_g.updateVar("c", 1);
  payoffMatrix_g.updateVar("gamma", 1);
  //   payoffMatrix_g.updateVar("lambda", 1);
  fmt::print("vars: {}\n", payoffMatrix_g.getVars());

  payoffMatrix_g.evalPayoffMatrix();
  cout << "after eval:" << endl;
  for (int r = 0; r < payoffMatrix_g.getRowNum(); r++) {
    for (int c = 0; c < payoffMatrix_g.getColNum(); c++) {
      for (int p = 0; p < payoffMatrix_g.getPlayerNum(); p++) {
        cout << payoffMatrix_g.getPayoffMatrix()[r][c][p] << ",";
      }
      cout << "\t";
    }
    cout << endl;
  }
  cout << "---------<<" << endl;

  cout << "--------->>" << endl;
  PayoffMatrix payoffMatrix_b("./BPayoffMatrix.csv");
  fmt::print("payoffMatrix_b: {}\n", payoffMatrix_b.getPayoffMatrixStr());
  // 输出需要赋值的所有变量
  fmt::print("payoffMatrix_b vars: {}\n", payoffMatrix_b.getVars());

  // vars: {b: 0, beta: 0, c: 0, gamma: 0, lambda: 0, }
  // 赋值后
  // payoffMatrix_b.updateVar("b", 1);
  // payoffMatrix_b.updateVar("beta", 4);
  // payoffMatrix_b.updateVar("c", 1);
  // payoffMatrix_b.updateVar("gamma", 1);
  payoffMatrix_b.setVars(payoffMatrix_g.getVars());
  //   payoffMatrix_b.updateVar("lambda", 1);
  cout << "after assign:" << endl;
  fmt::print("payoffMatrix_b vars: {}\n", payoffMatrix_b.getVars());

  payoffMatrix_b.evalPayoffMatrix();
  cout << "after eval:" << endl;
  for (int r = 0; r < payoffMatrix_b.getRowNum(); r++) {
    for (int c = 0; c < payoffMatrix_b.getColNum(); c++) {
      for (int p = 0; p < payoffMatrix_b.getPlayerNum(); p++) {
        cout << payoffMatrix_b.getPayoffMatrix()[r][c][p] << ",";
      }
      cout << "\t";
    }
    cout << endl;
  }

  cout << "---------<<" << endl;

  // 设置公共信息
  //   Player::addCommonInfo("lambda", 1);
  fmt::print("commonInfo: {}\n", Player::getCommonInfo());

  // 初始化两个博弈玩家
  vector<Action> donorActions;
  donorActions.push_back(Action("C", 0));
  donorActions.push_back(Action("D", 1));
  Player donor("donor", 0, donorActions);
  vector<Strategy> donorStrategies;
  donorStrategies.push_back(Strategy("C", 0));
  donorStrategies.push_back(Strategy("OC", 1));
  donorStrategies.push_back(Strategy("OD", 2));
  donorStrategies.push_back(Strategy("D", 3));
  donor.setStrategies(donorStrategies);
  donor.loadStrategy("./strategy");
  fmt::print("donorStrategies: {}\n", donor.getStrategyTables());
  // TODO: 设置初始策略
  donor.setStrategy("C");

  vector<Action> recipientActions;
  recipientActions.push_back(Action("C", 0));
  recipientActions.push_back(Action("D", 1));
  Player recipient("recipient", 0, recipientActions);
  vector<Strategy> recipientStrategies;
  recipientStrategies.push_back(Strategy("NR", 0));
  recipientStrategies.push_back(Strategy("SR", 1));
  recipientStrategies.push_back(Strategy("AR", 2));
  recipientStrategies.push_back(Strategy("UR", 3));
  recipient.setStrategies(recipientStrategies);

  recipient.loadStrategy("./strategy");
  fmt::print("recipientStrategies: {}\n", recipient.getStrategyTables());
  // TODO: 设置初始化策略
  recipient.setStrategy("NR");

  // 给声誉一个 0-1 的随机整数
  // 时间随机种子
  // unsigned seed = chrono::system_clock::now().time_since_epoch().count();
  // default_random_engine gen(seed);
  // uniform_int_distribution<int> dis(0, 1);
  // recipient.addVar(REPUTATION_STR, dis(gen));

  recipient.addVar(REPUTATION_STR, 1);
  fmt::print("recipient vars: {}\n", recipient.getVars());

  donor.setStrategy("OC");
  recipient.setStrategy("NR");
  recipient.updateVar(REPUTATION_STR, 0);
  
  for (int step = 0; step < stepNum; step++) {
    cout << endl << "-------------------- step " << step << endl;
    // 第一阶段 donor 行动
    Action donorAction = donor.donate(
        std::to_string((int)recipient.getVarValue(REPUTATION_STR)));
    fmt::print("donorStrategy:{0}, donorAction: {1}\n",
               donor.getStrategy().getName(), donorAction.getName());
    // 第二阶段 recipient 行动，记录本轮声望
    Action recipientAction = recipient.reward(donorAction.getName());
    double currentReputation = recipient.getVarValue(REPUTATION_STR);
    fmt::print("recipientStrategy:{0}, recipientAction: {1}\n",
               recipient.getStrategy().getName(), recipientAction.getName());
    // 第三阶段 更新recipient 的声誉
    double newReputation = norm.getReputation(donorAction, recipientAction);
    fmt::print("reputation : {} -> ", currentReputation);
    fmt::print("new: {} \n", newReputation);
    recipient.updateVar(REPUTATION_STR, newReputation);
    // 第四阶段 结算双方收益
    double donorPayoff, recipientPayoff;
    if (currentReputation == 1) {
      donorPayoff = payoffMatrix_g.getPayoff(donor.getStrategy(),
                                             recipient.getStrategy())[0];
      recipientPayoff = payoffMatrix_g.getPayoff(donor.getStrategy(),
                                                 recipient.getStrategy())[1];
    } else if (currentReputation == 0) {
      donorPayoff = payoffMatrix_b.getPayoff(donor.getStrategy(),
                                             recipient.getStrategy())[0];
      recipientPayoff = payoffMatrix_b.getPayoff(donor.getStrategy(),
                                                 recipient.getStrategy())[1];
    }
    donor.updateScore(donorPayoff);
    recipient.updateScore(recipientPayoff);
    fmt::print("donor:{0}, recipient:{1}\n", donorPayoff, recipientPayoff);
  }
  fmt::print("\nscore: donor{0}, recipient{1}", donor.getScore()/(double)stepNum, recipient.getScore()/(double)stepNum);

  return 0;
}