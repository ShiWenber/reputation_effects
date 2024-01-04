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

#include <chrono>
#include <climits>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
// #include <execution>
// #include <tbb/task.h>

#include <tbb/parallel_for.h>
#include <tbb/task_arena.h>

#include<indicators/cursor_control.hpp>
#include<indicators/progress_bar.hpp>

#include "Action.hpp"
#include "Norm.hpp"
#include "PayoffMatrix.hpp"
#include "Player.hpp"
#include "Strategy.hpp"

#define REPUTATION_STR "reputation"

using namespace std;
using namespace std::chrono;

/**
 * @brief 费米函数，用来计算策略转变概率
 *
 * @param payoff_current 现状收益
 * @param payoff_new 新策略收益
 * @param s
 * @return double
 */
double fermi(double payoff_current, double payoff_new, double s) {
  double res = 1 / (1 + exp((payoff_current - payoff_new) * s));
  // if (res > 0.97) {
  //   cout << payoff_current << "," << payoff_new << "," << s << endl;
  // }
  // cout << payoff_current << "," << payoff_new << "," << s << endl;
  // cout << res << endl;
  return res;
}

/**
 * @brief 应用转变
 *
 * @param strategyName2Id
 * @param player2StrategyChange
 */
void applyStrategyChange(
    unordered_map<string, set<int>> &strategyName2Id,
    unordered_map<int, pair<string, string>> player2StrategyChange) {
  for (auto it = player2StrategyChange.begin();
       it != player2StrategyChange.end(); it++) {
    // it->second.first 是 from
    // it->second.second 是 to
    strategyName2Id[it->second.first].erase(it->first);
    strategyName2Id[it->second.second].insert(it->first);
  }
}

/**
 * @brief  统计每个策略对的人数同时可以生成log行:
 * step, C-NR, C-SR, C-AR, C-UR, OC-NR, OC-SR, OC-AR, OC-UR, OD-NR, OD-SR,
 * OD-AR, OD-UR, D-NR, D-SR, D-AR, D-UR, C, OC, OD, D, NR, SR, AR, UR
 *
 *
 * @param donors
 * @param recipients
 * @param donorStrategies
 * @param recipientStrategies
 * @param strategyName2DonorId
 * @param strategyName2RecipientId
 * @param population
 * @param step
 * @param print
 * @return string
 */
string printStatistics(vector<Player> donors, vector<Player> recipients,
                       vector<Strategy> donorStrategies,
                       vector<Strategy> recipientStrategies,
                       unordered_map<string, set<int>> strategyName2DonorId,
                       unordered_map<string, set<int>> strategyName2RecipientId,
                       int population, int step, bool print) {
  double population_double = (double)population;
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
        fmt::print("{0}: {1}, ", key,
                   strategyPair2Num[key] / population_double);
      }
    }
    fmt::print("\n");
    for (Strategy donorS : donorStrategies) {
      key = donorS.getName();
      fmt::print("{0}: {1}, ", key,
                 strategyName2DonorId[key].size() / population_double);
    }
    fmt::print("\n");
    for (Strategy recipientS : recipientStrategies) {
      key = recipientS.getName();
      fmt::print("{0}: {1}, ", key,
                 strategyName2RecipientId[key].size() / population_double);
    }
  } else {
    for (Strategy donorS : donorStrategies) {
      for (Strategy recipientS : recipientStrategies) {
        key = donorS.getName() + "-" + recipientS.getName();
        logLine += "," + to_string(strategyPair2Num[key] / population_double);
      }
    }
    for (Strategy donorS : donorStrategies) {
      key = donorS.getName();
      logLine +=
          "," + to_string(strategyName2DonorId[key].size() / population_double);
    }
    for (Strategy recipientS : recipientStrategies) {
      key = recipientS.getName();
      logLine += "," + to_string(strategyName2RecipientId[key].size() /
                                 population_double);
    }
  }
  return logLine;
}

/**
 * @brief 须输入参数案例
 *   // 博弈参数
 * int stepNum = 1000;
 * int population = 200;  // 人数，这里作为博弈对数，100表示100对博弈者
 * double s = 1;          // 费米函数参数
 *
 * int b = 4;      // 公共参数
 * int beta = 3;   // 公共参数
 * int c = 1;      // 公共参数
 * int gamma = 1;  // 公共参数
 * int mu = 0.05;  // 动作突变率
 * int normId = 10;
 *
 *
 *
 * @return int
 */
void func(int stepNum, int population, double s, int b, int beta, int c,
          int gamma, double mu, int normId, int updateStepNum) {
    using namespace indicators;

  // Hide cursor
  show_console_cursor(false);

  indicators::ProgressBar bar{
    option::BarWidth{50},
    option::Start{" ["},
    option::Fill{"*"},
    option::Lead{"*"},
    option::Remainder{"-"},
    option::End{"]"},
    option::PrefixText{"Progress: "},
    // option::ForegroundColor{Color::yellow},
    option::ShowElapsedTime{true},
    option::ShowRemainingTime{true},
    option::FontStyles{std::vector<FontStyle>{FontStyle::bold}}
  };



  string normName = "norm" + to_string(normId);

  cout << "---" << endl;

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

  // for (int normNumber = 1; normNumber < 16; normNumber++) {
  //   normName = "norm" + to_string(normNumber);

  // 加载norm
  string normPath = "./norm/" + normName + ".csv";
  Norm norm(normPath);
  fmt::print("norm: {}\n", norm.getNormTableStr());

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
    // temp_recipient.updateVar(REPUTATION_STR, dis_reputation(gen_reputation));
    // 固定声誉为1
    temp_recipient.updateVar(REPUTATION_STR, 1);

    strategyName2RecipientId[temp_recipient.getStrategy().getName()].insert(i);
    recipients.push_back(temp_recipient);
  }

  printStatistics(donors, recipients, donorStrategies, recipientStrategies,
                  strategyName2DonorId, strategyName2RecipientId, population, 0,
                  true);

  // donor.setStrategy("OC");
  // recipient.setStrategy("NR");
  // recipient.updateVar(REPUTATION_STR, 0);

  // TODO: log对象
  string logDir = "./log/" + normName + "/";
  // 判断路径存不存在，如果不存在就新建
  if (!filesystem::exists(logDir)) {
    filesystem::create_directory(logDir);
  }

  string logName = "stepNum-" + to_string(stepNum) + "_population-" +
                   to_string(population) + "_s-" + to_string(s) + "_b-" +
                   to_string(b) + "_beta-" + to_string(beta) + "_c-" +
                   to_string(c) + "_gamma-" + to_string(gamma) + "_mu-" +
                   to_string(mu) + "_norm-" + normName + "_uStepN-" +
                   to_string(updateStepNum) + ".csv";
  auto out = fmt::output_file(logDir + logName);
  // 生成表头
  string line = "step";
  for (Strategy donorS : donorStrategies) {
    for (Strategy recipientS : recipientStrategies) {
      line += "," + donorS.getName() + "-" + recipientS.getName();
    }
  }
  for (Strategy donorS : donorStrategies) {
    line += "," + donorS.getName();
  }
  for (Strategy recipientS : recipientStrategies) {
    line += "," + recipientS.getName();
  }
  out.print("{}\n", line);

  string logLine = printStatistics(
      donors, recipients, donorStrategies, recipientStrategies,
      strategyName2DonorId, strategyName2RecipientId, population, 0, false);
  out.print("{}\n", logLine);

  // 1. 固定博弈者对交互 TODO: 不固定博弈者对轮流交互取平均
  for (int step = 0; step < stepNum; step++) {
    bar.set_progress((double)step / stepNum * 100);

    unordered_map<int, pair<string, string>> donor2StrateChange;
    unordered_map<int, pair<string, string>> recipientId2StrateChange;

    // for_each 并行
    // vector<int> ids(population);
    // std::for_each(std::execution::par, ids.begin(), ids.end(), [&](int i) {
    for (int i = 0; i < population; i++) {  // --------------------->>1

      // 判断recipient的策略是否存在

      // cout << endl << "-------------------- step " << step << endl;
      // 第一阶段 donors[i] 行动
      Action donorAction = donors[i].donate(
          std::to_string((int)recipients[i].getVarValue(REPUTATION_STR)), mu);
      // fmt::print("donorStrategy:{0}, donorAction: {1}\n",
      //            donors[i].getStrategy().getName(), donorAction.getName());
      // 第二阶段 recipients[i] 行动，记录本轮声望
      Action recipientAction = recipients[i].reward(donorAction.getName(), mu);
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
    }  // ----------<<1
    // });  // 并行

    // 未到更新轮数或者step为0，跳过更新策略
    if (step == 0 || step % updateStepNum != 0) {
      continue;
    }

    // 更新策略
    for (int i = 0; i < population; i++) {  // --------------------->>2
      // 第五阶段 更新双方策略
      // 更新donor策略

      // 抽取一个种群中的其他人，将其策略更新
      // 如果抽到自己就重新抽取
      int randId = 0;
      do {
        randId = donors[i].getRandomInt(0, population - 1);
      } while (randId == i);

      // 初始化为int的极小值
      int newPayoff = INT_MIN;
      newPayoff = donors[randId].getDeltaScore();

      if (newPayoff == INT_MIN) {
        cerr << "newPayoff is INT_MIN" << endl;
        throw "newPayoff is INT_MIN";
      }

      // 费米更新
      // 抛出一个随机概率，如果小于费米概率就更新为新策略
      // TODO: 可以改为增量式的，由set自己维护自己的平均收益可以减少一层循环
      if (donors[i].getProbability() <
          fermi(donors[i].getDeltaScore(), newPayoff, s)) {
        // 记录转变
        donor2StrateChange[i] =
            make_pair(donors[i].getStrategy().getName(),
                      donors[randId].getStrategy().getName());

        donors[i].setStrategy(donors[randId].getStrategy());

      }

      // 更新recipient策略
      do {
        randId = recipients[i].getRandomInt(0, population - 1);
      } while (randId == i);

      // 初始化为int的极小值
      newPayoff = INT_MIN;
      newPayoff = recipients[randId].getDeltaScore();

      if (newPayoff == INT_MIN) {
        cout << "newPayoff is INT_MIN" << endl;
        throw "newPayoff is INT_MIN";
      }

      if (recipients[i].getProbability() <
          fermi(recipients[i].getDeltaScore(), newPayoff, s)) {
        // 记录转变
        recipientId2StrateChange[i] =
            make_pair(recipients[i].getStrategy().getName(),
                      recipients[randId].getStrategy().getName());
        recipients[i].setStrategy(recipients[randId].getStrategy());

      }
      // 更新后清空deltaScore
      donors[i].clearDeltaScore();
      recipients[i].clearDeltaScore();
    }  // --------------<<2

    // 第六阶段，本轮末尾应用所有转变
    applyStrategyChange(strategyName2DonorId, donor2StrateChange);
    applyStrategyChange(strategyName2RecipientId, recipientId2StrateChange);

    // 生成log
    out.print("{}\n", printStatistics(donors, recipients, donorStrategies,
                                      recipientStrategies, strategyName2DonorId,
                                      strategyName2RecipientId, population,
                                      step + 1, false));
  }

  // for (int i = 0; i < population; i++) {
  //   fmt::print("\nscore: donors[{0}]: {1}, recipients[{0}]: {2}\n", i,
  //              donors[i].getScore(), recipients[i].getScore());
  // }

  printStatistics(donors, recipients, donorStrategies, recipientStrategies,
                  strategyName2DonorId, strategyName2RecipientId, population,
                  stepNum, true);

  // Show cursor
  show_console_cursor(true);

}

int main() {
  // 记录运行时间
  system_clock::time_point start = chrono::system_clock::now();
  tbb::task_arena arena(16);  // 创建一个并行度为4的arena

  // 博弈参数
  int stepNum = 3000;
  int population = 400;  // 人数，这里作为博弈对数，100表示100对博弈者
  double s = 1;          // 费米函数参数

  int b = 4;      // 公共参数
  int beta = 3;   // 公共参数
  int c = 1;      // 公共参数
  int gamma = 1;  // 公共参数
  double mu = 0.05;  // 动作突变率
  int normId = 10;
  int updateStepNum = 1;  // 表示每隔多少步更新一次策略

  // 调试使用
  func(stepNum, population, s, b, beta, c, gamma, mu, normId, updateStepNum);

  // 多线程加速
  // arena.execute([&]() {
  //   // for (int normId = 0; normId < 16; normId++) {
  //   //   func(stepNum, population, s, b, beta, c, gamma, mu, normId,
  //   //        updateStepNum);
  //   // }

  //   for (int stepNum = 2000;  stepNum < 2050; stepNum++) {
  //     func(stepNum, population, s, b, beta, c, gamma, mu, normId,
  //          updateStepNum);
  //   }


  //   // tbb::parallel_for(0, 16, [&](int normId){
  //   //   func(stepNum, population, s, b, beta, c, gamma, mu, normId,
  //   //   updateStepNum);
  //   // });
  // });

  // for (int normId = 0; normId < 16; normId++) {
  //   func(stepNum, population, s, b, beta, c, gamma, mu, normId);
  // }

  system_clock::time_point end = system_clock::now();
  cout << "\ntime: " << duration_cast<microseconds>(end - start).count() / 1e6
       << "s" << endl;
  return 0;
}