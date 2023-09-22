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
#include <fmt/os.h>
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
 * @param payoff_current 现状收益
 * @param payoff_new 新策略收益
 * @param s
 * @return double
 */
double fermi(double payoff_current, double payoff_new, double s) {
  double res = 1 / (1 + exp((payoff_current - payoff_new) / s));
  // if (res > 0.97) {
  //   cout << payoff_current << "," << payoff_new << "," << s << endl;
  // }
  // cout << res << endl;
  return res;
}

string printStatistics(vector<Player> donors, vector<Player> recipients,
                       vector<Strategy> donorStrategies,
                       vector<Strategy> recipientStrategies, int population,
                       int step, bool print) {
  // 统计每个策略对的人数
  //  0: C-NR   1: C-SR   2: C-AR    3: C-UR
  //  4: OC-NR  5: OC-SR  6: OC-AR   7: OC-UR
  //  8: OD-NR  9: OD-SR  10: OD-AR  11: OD-UR
  //  12: D-NR 13: D-SR   14: D-AR   15: D-UR
  unordered_map<string, int> strategyPair2Num;
  string key;
  for (int i = 0; i < population; i++) {
    key = donors[i].getStrategy().getName() + "-" +
          recipients[i].getStrategy().getName();
    strategyPair2Num[key]++;
  }

  string logLine = to_string(step);

  if (print) {
    fmt::print("strategyPair2Num: {}\n", strategyPair2Num);
    for (Strategy donorS : donorStrategies) {
      for (Strategy recipientS : recipientStrategies) {
        key = donorS.getName() + "-" + recipientS.getName();
        fmt::print("{0}: {1}, ", key, strategyPair2Num[key]);
      }
    }
  } else {
    for (Strategy donorS : donorStrategies) {
      for (Strategy recipientS : recipientStrategies) {
        key = donorS.getName() + "-" + recipientS.getName();
        logLine += "," + to_string(strategyPair2Num[key]);
      }
    }
  }
  return logLine;
}

int main() {
  // 博弈参数
  int stepNum = 1000;
  int population = 400;  // 人数，这里作为博弈对数，100表示100对博弈者
  int s = 1;             // 费米函数参数

  int b = 4;      // 公共参数
  int beta = 3;   // 公共参数
  int c = 1;      // 公共参数
  int gamma = 1;  // 公共参数

  string normName = "norm";

  cout << "---" << endl;

  // 加载norm
  string normPath = "./" + normName + ".csv";
  Norm norm(normPath);
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
  payoffMatrix_g.updateVar("b", b);
  payoffMatrix_g.updateVar("beta", beta);
  payoffMatrix_g.updateVar("c", c);
  payoffMatrix_g.updateVar("gamma", gamma);
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

  // 以上面的博弈者对作模板生成population对博弈者
  vector<Player> donors;
  vector<Player> recipients;
  // 使用不同的时间种子生成随机数
  unsigned seed_don = chrono::system_clock::now()
                          .time_since_epoch()
                          .count();  // 加上指定值避免紧挨的种子相同
  default_random_engine gen_don(seed_don);
  uniform_int_distribution<int> dis_don(0, donorStrategies.size() - 1);

  unsigned seed_rec =
      chrono::system_clock::now().time_since_epoch().count() + 1;
  default_random_engine gen_rec(seed_rec);
  uniform_int_distribution<int> dis_rec(0, recipientStrategies.size() - 1);

  unsigned seed_reputation =
      chrono::system_clock::now().time_since_epoch().count() + 2;
  default_random_engine gen_reputation(seed_reputation);
  uniform_int_distribution<int> dis_reputation(0, 1);

  unordered_map<string, set<int>> strategyName2DonorId;
  unordered_map<string, set<int>> strategyName2RecipientId;
  for (int i = 0; i < population; i++) {
    Player temp_donor(donor);
    Player temp_recipient(recipient);

    temp_donor.setStrategy(donorStrategies[dis_don(gen_don)]);
    strategyName2DonorId[temp_donor.getStrategy().getName()].insert(i);
    donors.push_back(temp_donor);

    temp_recipient.setStrategy(recipientStrategies[dis_rec(gen_rec)]);
    temp_recipient.updateVar(REPUTATION_STR, dis_reputation(gen_reputation));
    strategyName2RecipientId[temp_recipient.getStrategy().getName()].insert(i);
    recipients.push_back(temp_recipient);
  }

  printStatistics(donors, recipients, donorStrategies, recipientStrategies,
                  population, 0, true);

  // donor.setStrategy("OC");
  // recipient.setStrategy("NR");
  // recipient.updateVar(REPUTATION_STR, 0);

  // TODO: log对象
  string logPath = "stepNum-" + to_string(stepNum) + "_population-" +
                   to_string(population) + "_s-" + to_string(s) + "_b-" +
                   to_string(b) + "_beta-" + to_string(beta) + "_c-" +
                   to_string(c) + "_gamma-" + to_string(gamma) + ".csv";
  auto out = fmt::output_file(logPath);
  // 生成表头
  string line = "step";
  for (Strategy donorS : donorStrategies) {
    for (Strategy recipientS : recipientStrategies) {
      line += "," + donorS.getName() + "-" + recipientS.getName();
    }
  }
  out.print("{}\n", line);

  string logLine = printStatistics(donors, recipients, donorStrategies,
                                   recipientStrategies, population, 0, false);
  out.print("{}\n", logLine);

  // 1. 固定博弈者对交互 TODO: 不固定博弈者对轮流交互取平均

  for (int step = 0; step < stepNum; step++) {
    for (int i = 0; i < population; i++) {
      // cout << endl << "-------------------- step " << step << endl;
      // 第一阶段 donors[i] 行动
      Action donorAction = donors[i].donate(
          std::to_string((int)recipients[i].getVarValue(REPUTATION_STR)));
      // fmt::print("donorStrategy:{0}, donorAction: {1}\n",
      //            donors[i].getStrategy().getName(), donorAction.getName());
      // 第二阶段 recipients[i] 行动，记录本轮声望
      Action recipientAction = recipients[i].reward(donorAction.getName());
      double currentReputation = recipients[i].getVarValue(REPUTATION_STR);
      // fmt::print("recipientStrategy:{0}, recipientAction: {1}\n",
      //            recipients[i].getStrategy().getName(),
      //            recipientAction.getName());
      // 第三阶段 更新recipient 的声誉
      double newReputation = norm.getReputation(donorAction, recipientAction);
      // fmt::print("reputation : {} -> ", currentReputation);
      // fmt::print("new: {} \n", newReputation);
      recipients[i].updateVar(REPUTATION_STR, newReputation);
      // 第四阶段 结算双方收益
      double donorPayoff, recipientPayoff;
      if (currentReputation == 1) {
        donorPayoff = payoffMatrix_g.getPayoff(donors[i].getStrategy(),
                                               recipients[i].getStrategy())[0];
        recipientPayoff = payoffMatrix_g.getPayoff(
            donors[i].getStrategy(), recipients[i].getStrategy())[1];
      } else if (currentReputation == 0) {
        donorPayoff = payoffMatrix_b.getPayoff(donors[i].getStrategy(),
                                               recipients[i].getStrategy())[0];
        recipientPayoff = payoffMatrix_b.getPayoff(
            donors[i].getStrategy(), recipients[i].getStrategy())[1];
      }
      donors[i].updateScore(donorPayoff);
      recipients[i].updateScore(recipientPayoff);
      // fmt::print("donors[i]:{0}, recipients[i]:{1}\n", donorPayoff,
      //            recipientPayoff);
      // 第五阶段 更新双方策略
      // 更新donor策略
      vector<Strategy> donorAlterStrategy;
      // 设置备选策略
      for (int j = 0; j < donorStrategies.size(); j++) {
        // 排除当前策略和set.size()为0的策略
        if (donorStrategies[j].getName() != donors[i].getStrategy().getName() &&
            strategyName2DonorId[donorStrategies[j].getName()].size() != 0) {
          donorAlterStrategy.push_back(donorStrategies[j]);
        }
      }

      Strategy newDonorStrategy =
          donors[i].getRandomOtherStrategy(donorAlterStrategy);
      // 从采用该策略的人中求平均值
      double sum = 0;
      for (int donorId : strategyName2DonorId[newDonorStrategy.getName()]) {
        sum += donors[donorId].getScore();
      }

      // 判断 size() 防止除0
      if (strategyName2DonorId[newDonorStrategy.getName()].size() == 0) {
        throw "size is 0, will dive 0";
      }
      double avgDonorScore =
          sum / strategyName2DonorId[newDonorStrategy.getName()].size();

      // 费米更新
      // 抛出一个随机概率，如果大于费米概率就更新为新策略
      // TODO: 可以改为增量式的，由set自己维护自己的平均收益可以减少一层循环
      if (donors[i].getProbability() <
          fermi(donors[i].getScore(), avgDonorScore, s)) {
        // 更新策略并更新每个策略的set
        // 从原来的set中删除
        // fmt::print("----\nold: {}",
        // strategyName2DonorId[donors[i].getStrategy().getName()]);
        strategyName2DonorId[donors[i].getStrategy().getName()].erase(i);
        // fmt::print("-> {}\n",
        // strategyName2DonorId[donors[i].getStrategy().getName()]);
        // 在新的set中添加
        // fmt::print("old: {}",
        // strategyName2DonorId[newDonorStrategy.getName()]);
        strategyName2DonorId[newDonorStrategy.getName()].insert(i);
        // fmt::print("-> {}\n",
        // strategyName2DonorId[newDonorStrategy.getName()]);
        donors[i].setStrategy(newDonorStrategy);
      }

      // 设置备选策略
      vector<Strategy> recipientAlterStrategy;
      // 设置备选策略
      for (int j = 0; j < recipientStrategies.size(); j++) {
        // 排除当前策略和set.size()为0的策略
        if (recipientStrategies[j].getName() !=
                recipients[i].getStrategy().getName() &&
            strategyName2RecipientId[recipientStrategies[j].getName()].size() !=
                0) {
          recipientAlterStrategy.push_back(recipientStrategies[j]);
        }
      }

      // 更新recipient策略
      Strategy newRecipientStrategy =
          recipients[i].getRandomOtherStrategy(recipientAlterStrategy);
      // 从采用该策略的人中求平均值
      sum = 0;
      for (int recipientId :
           strategyName2RecipientId[newRecipientStrategy.getName()]) {
        sum += recipients[recipientId].getScore();
      }

      double avgRecipientScore =
          sum / strategyName2RecipientId[newRecipientStrategy.getName()].size();
      if (recipients[i].getProbability() <
          fermi(recipients[i].getScore(), avgRecipientScore, s)) {
        // 更新策略并更新每个策略的set
        // 从原来的set中删除
        // fmt::print("----\nold: {}",
        // strategyName2RecipientId[recipients[i].getStrategy().getName()]);
        strategyName2RecipientId[recipients[i].getStrategy().getName()].erase(
            i);
        // fmt::print("-> {}\n",
        // strategyName2RecipientId[recipients[i].getStrategy().getName()]);
        // 在新的set中添加
        // fmt::print("old: {}",
        // strategyName2RecipientId[newRecipientStrategy.getName()]);
        strategyName2RecipientId[newRecipientStrategy.getName()].insert(i);
        // fmt::print("-> {}\n",
        // strategyName2RecipientId[newRecipientStrategy.getName()]);
        recipients[i].setStrategy(newRecipientStrategy);
      }
    }
    // 生成log
    out.print("{}\n",
              printStatistics(donors, recipients, donorStrategies,
                              recipientStrategies, population, step+1, false));
  }

  for (int i = 0; i < population; i++) {
    fmt::print("\nscore: donors[{0}]: {1}, recipients[{0}]: {2}\n", i,
               donors[i].getScore(), recipients[i].getScore());
  }

  printStatistics(donors, recipients, donorStrategies, recipientStrategies,
                  population, stepNum, true);

  return 0;
}