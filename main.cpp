/**
 * @file main.cpp
 * @author ShiWenber (1210169842@qq.com)
 * @brief the simulation of the evolution of cooperation based on the static
 * payoff matrix (combined the reputation and norm)
 * @version 0.1
 * @date 2023-09-12
 *
 * @copyright Copyright (c) 2023
 *
 */

#define CREATE_BAR(n)                                                   \
  ProgressBar bar##n {                                                  \
    option::BarWidth{50}, option::Start{" ["}, option::Fill{"*"},       \
        option::Lead{"*"}, option::Remainder{"-"}, option::End{"]"},    \
        option::PrefixText{"Progress: " + std::to_string(n) + " "},     \
        option::ShowElapsedTime{true}, option::ShowRemainingTime{true}, \
        option::FontStyles {                                            \
      std::vector<FontStyle> { FontStyle::bold }                        \
    }                                                                   \
  }

#include <cmath>
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

#include <indicators/cursor_control.hpp>
#include <indicators/dynamic_progress.hpp>
#include <indicators/progress_bar.hpp>
#include <numeric>

#include "Action.hpp"
#include "Norm.hpp"
#include "PayoffMatrix.hpp"
#include "Player.hpp"
#include "Strategy.hpp"

#define REPUTATION_STR "reputation"

using namespace std;
using namespace indicators;
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

double getAvgPayoff(const Strategy& donorStrategy,const Strategy& recipientStrategy,
                    const PayoffMatrix& payoffMatrix,
                    const unordered_map<string, set<int>>& strategyName2donorId,
                    const unordered_map<string, set<int>>& strategyName2recipientId,
                    int population) {
  double eval_donor = 0;
  double eval_recipient = 0;
  vector<double> players_payoff =
      payoffMatrix.getPayoff(donorStrategy, recipientStrategy);
  double eval_same =
      accumulate(players_payoff.begin(), players_payoff.end(), 0.0) / 2;
  for (int j = 0; j < 4; j++) {
    Strategy d_stra = payoffMatrix.getRowStrategies()[j];
    Strategy r_stra = payoffMatrix.getColStrategies()[j];
    eval_donor += payoffMatrix.getPayoff(donorStrategy, r_stra)[0] *
                  strategyName2recipientId.at(r_stra.getName()).size() * 0.5;
    eval_recipient += payoffMatrix.getPayoff(d_stra, recipientStrategy)[1] *
                      strategyName2donorId.at(d_stra.getName()).size() * 0.5;
  }
  return (1.0 / (population - 1)) * (eval_donor + eval_recipient - eval_same);
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
string printStatistics(const vector<Player>& donors, const vector<Player>& recipients,
                       const vector<Strategy>& donorStrategies,
                       const vector<Strategy>& recipientStrategies,
                       const unordered_map<string, set<int>>& strategyName2DonorId,
                       const unordered_map<string, set<int>>& strategyName2RecipientId,
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
                 strategyName2DonorId.at(key).size() / population_double);
    }
    fmt::print("\n");
    for (Strategy recipientS : recipientStrategies) {
      key = recipientS.getName();
      fmt::print("{0}: {1}, ", key,
                 strategyName2RecipientId.at(key).size() / population_double);
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
          "," + to_string(strategyName2DonorId.at(key).size() / population_double);
    }
    for (Strategy recipientS : recipientStrategies) {
      key = recipientS.getName();
      logLine += "," + to_string(strategyName2RecipientId.at(key).size() /
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
          int gamma, double mu, int normId, int updateStepNum, double p0,
          ProgressBar *bar = nullptr, bool turn_up_progress_bar = false,
          DynamicProgress<ProgressBar> *dynamic_bar = nullptr,
          bool turn_up_dynamic_bar = false, int dynamic_bar_id = 0) {
  // // Hide cursor
  // show_console_cursor(false);

  // indicators::ProgressBar bar{
  //     option::BarWidth{50}, option::Start{" ["}, option::Fill{"*"},
  //     option::Lead{"*"}, option::Remainder{"-"}, option::End{"]"},
  //     option::PrefixText{"Progress: "},
  //     // option::ForegroundColor{Color::yellow},
  //     option::ShowElapsedTime{true}, option::ShowRemainingTime{true},
  //     option::FontStyles{std::vector<FontStyle>{FontStyle::bold}}};

  string normName = "norm" + to_string(normId);

  // 加载payoffMatrix
  // cout << "---------->>" << endl;
  PayoffMatrix payoffMatrix("./payoffMatrix/PayoffMatrix" + to_string(normId) +
                            ".csv");
  // fmt::print("payoffMatrix_g: {}\n", payoffMatrix_g.getPayoffMatrixStr());
  // // 输出需要赋值的所有变量
  // fmt::print("vars: {}\n", payoffMatrix_g.getVars());
  // cout << "after assign:" << endl;

  // assign vars
  payoffMatrix.updateVar("b", b);
  payoffMatrix.updateVar("beta", beta);
  payoffMatrix.updateVar("c", c);
  payoffMatrix.updateVar("gamma", gamma);
  payoffMatrix.updateVar("p", p0);

  payoffMatrix.evalPayoffMatrix();

  // cout << "after eval:" << endl;
  // for (int r = 0; r < payoffMatrix_g.getRowNum(); r++) {
  //   for (int c = 0; c < payoffMatrix_g.getColNum(); c++) {
  //     for (int p = 0; p < payoffMatrix_g.getPlayerNum(); p++) {
  //       cout << payoffMatrix_g.getPayoffMatrix()[r][c][p] << ",";
  //     }
  //     cout << "\t";
  //   }
  //   cout << endl;
  // }
  // cout << "---------<<" << endl;

  // initialize two players
  vector<Action> donorActions;
  donorActions.push_back(Action("C", 0));
  donorActions.push_back(Action("D", 1));
  Player donor("donor", 0, donorActions);
  vector<Strategy> donorStrategies = payoffMatrix.getRowStrategies();
  donor.setStrategies(donorStrategies);
  donor.loadStrategy("./strategy");
  // fmt::print("donorStrategies: {}\n", donor.getStrategyTables());
  donor.setStrategy("C");

  vector<Action> recipientActions;
  recipientActions.push_back(Action("C", 0));
  recipientActions.push_back(Action("D", 1));
  Player recipient("recipient", 0, recipientActions);
  vector<Strategy> recipientStrategies = payoffMatrix.getColStrategies();
  recipient.setStrategies(recipientStrategies);

  recipient.loadStrategy("./strategy");
  // fmt::print("recipientStrategies: {}\n", recipient.getStrategyTables());
  recipient.setStrategy("NR");

  // initialize recipient's reputation
  recipient.addVar(REPUTATION_STR, 1);
  // fmt::print("recipient vars: {}\n", recipient.getVars());

  // load norm
  string normPath = "./norm/" + normName + ".csv";
  Norm norm(normPath);
  // fmt::print("norm: {}\n", norm.getNormTableStr());

  // the two players are used as templates to generate population players
  vector<Player> donors;
  vector<Player> recipients;
  // time seed
  unsigned seed_don = chrono::system_clock::now().time_since_epoch().count();
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

  unsigned seed_probability =
      chrono::system_clock::now().time_since_epoch().count() + 3;
  mt19937 gen_probability(seed_probability);
  uniform_real_distribution<double> dis_probability(0, 1);

  // record the strategy distribution of the population
  unordered_map<string, set<int>> strategyName2DonorId;
  unordered_map<string, set<int>> strategyName2RecipientId;
  // judge if population can be divided by donorStrategies.size()
  assert(population % donorStrategies.size() == 0);

  // reputation init vector
  vector<int> reputation_value(ceil(population * (1 - p0)), 0);
  vector<int> good_rep_value((int)population * p0, 1);
  assert(reputation_value.size() + good_rep_value.size() == population);

  reputation_value.insert(reputation_value.end(), good_rep_value.begin(),
                          good_rep_value.end());
  random_shuffle(reputation_value.begin(), reputation_value.end());

  // strategy init vector, each strategy pair has the same number of players
  vector<int> donor_stra_id;
  vector<int> recipient_stra_id;
  for (int i = 0; i < donorStrategies.size(); i++) {
    donor_stra_id.insert(donor_stra_id.end(),
                         population / (double)donorStrategies.size(), i);
    for (int j = 0; j < recipientStrategies.size(); j++) {
      recipient_stra_id.insert(recipient_stra_id.end(),
                               (population / (double)donorStrategies.size()) /
                                   (double)recipientStrategies.size(),
                               j);
    }
  }
  assert(donor_stra_id.size() == population &&
         recipient_stra_id.size() == population);
  vector<pair<int, int>> stra_id_pairs;
  for (int i = 0; i < donor_stra_id.size(); i++) {
    stra_id_pairs.push_back(make_pair(donor_stra_id[i], recipient_stra_id[i]));
  }
  random_shuffle(stra_id_pairs.begin(), stra_id_pairs.end());

  // initialize population players that have different strategies, and each
  // strategy has the same number of players
  for (int i = 0; i < population; i++) {
    Player temp_donor(donor);
    Player temp_recipient(recipient);
    // int donor_stra_i = donor_stra_id[i];
    // int recipient_stra_i = recipient_stra_id[i];
    auto [donor_stra_i, recipient_stra_i] = stra_id_pairs[i];

    temp_donor.setStrategy(donorStrategies[donor_stra_i]);
    strategyName2DonorId[temp_donor.getStrategy().getName()].insert(i);
    donors.push_back(temp_donor);

    temp_recipient.setStrategy(recipientStrategies[recipient_stra_i]);
    temp_recipient.updateVar(REPUTATION_STR, reputation_value[i]);
    // // 固定声誉为1
    // temp_recipient.updateVar(REPUTATION_STR, 1);

    strategyName2RecipientId[temp_recipient.getStrategy().getName()].insert(i);
    recipients.push_back(temp_recipient);
  }

  // printStatistics(donors, recipients, donorStrategies, recipientStrategies,
  //                 strategyName2DonorId, strategyName2RecipientId, population,
  //                 0, false);

  // log
  string logDir = "./log/" + normName + "/";
  // judge if the path exists, if not, create it
  if (!filesystem::exists(logDir)) {
    filesystem::create_directory(logDir);
  }

  string logName = "stepNum-" + to_string(stepNum) + "_population-" +
                   to_string(population) + "_s-" + to_string(s) + "_b-" +
                   to_string(b) + "_beta-" + to_string(beta) + "_c-" +
                   to_string(c) + "_gamma-" + to_string(gamma) + "_mu-" +
                   to_string(mu) + "_norm-" + normName + "_uStepN-" +
                   to_string(updateStepNum) + "_p0-" + to_string(p0) + ".csv";
  auto out = fmt::output_file(logDir + logName);
  // generate header
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

  uniform_int_distribution<int> dis(0, population - 1);
  for (int step = 0; step < stepNum; step++) {
    // update progress bar
    if (turn_up_progress_bar) {
      // (*bar).set_progress((double)step / stepNum * 100);
      // 只在进度条每增加1%时更新一次
      // only update once when the progress bar increases by 1%
      if (step % (stepNum / 100) == 0) {
        (*bar).tick();
      }
    } else if (turn_up_dynamic_bar) {
      if (step % (stepNum / 100) == 0) {
        (*dynamic_bar)[dynamic_bar_id].tick();
      }
    }

    // // record the strategy change of the population to update
    // strategyName2donorId and strategyName2recipientId unordered_map<int,
    // pair<string, string>> donor2StrateChange; unordered_map<int, pair<string,
    // string>> recipientId2StrateChange;

    // The random number of 0-population is extracted
    int focul_i = dis(gen_don);
    int rolemodel_i = dis(gen_rec);
    // to prevent the same person from being drawn
    while (focul_i == rolemodel_i) {
      focul_i = dis(gen_don);
      rolemodel_i = dis(gen_rec);
    }

    // mutation probability to explore other strategies randomly
    double p = dis_probability(gen_probability);
    assert(p >= 0 && p <= 1);
    // there is a probability of mu to explore other strategies randomly
    if (p < mu) {
      // update the focul's strategy
      int randId_d = 0;
      int randId_r = 0;
      do {
        randId_d = donors[focul_i].getRandomInt(0, donorStrategies.size() - 1);
        randId_r =
            recipients[focul_i].getRandomInt(0, recipientStrategies.size() - 1);
      } while (randId_d == donors[focul_i].getStrategy().getId() &&
               randId_r == recipients[focul_i].getStrategy().getId());

      //   // record the strategy change of the population to update
      //   distribution after the step ends donor2StrateChange[focul_i] =
      //       make_pair(donors[focul_i].getStrategy().getName(),
      //                 donorStrategies[randId].getName());

      strategyName2DonorId[donors[focul_i].getStrategy().getName()].erase(
          focul_i);
      donors[focul_i].setStrategy(donorStrategies[randId_d]);
      strategyName2DonorId[donorStrategies[randId_d].getName()].insert(focul_i);

      //   // record the strategy change of the population to update
      //   distribution after the step ends
      //   recipientId2StrateChange[rolemodel_i] =
      //       make_pair(recipients[rolemodel_i].getStrategy().getName(),
      //                 recipientStrategies[randId].getName());
      strategyName2RecipientId[recipients[focul_i].getStrategy().getName()]
          .erase(focul_i);
      recipients[focul_i].setStrategy(recipientStrategies[randId_r]);
      strategyName2RecipientId[recipientStrategies[randId_r].getName()].insert(
          focul_i);
    } else {
      Strategy rolemodel_donorStrategy = donors[rolemodel_i].getStrategy();
      Strategy rolemodel_recipientStrategy =
          recipients[rolemodel_i].getStrategy();
      double rolemodel_payoff = getAvgPayoff(
          rolemodel_donorStrategy, rolemodel_recipientStrategy, payoffMatrix,
          strategyName2DonorId, strategyName2RecipientId, population);

      Strategy focul_donorStrategy = donors[focul_i].getStrategy();
      Strategy focul_recipientStrategy = recipients[focul_i].getStrategy();
      double focul_payoff = getAvgPayoff(
          focul_donorStrategy, focul_recipientStrategy, payoffMatrix,
          strategyName2DonorId, strategyName2RecipientId, population);

      // fermi
      if (dis_probability(gen_probability) <
          fermi(focul_payoff, rolemodel_payoff, s)) {
        strategyName2DonorId[donors[focul_i].getStrategy().getName()].erase(
            focul_i);
        donors[focul_i].setStrategy(rolemodel_donorStrategy);
        strategyName2DonorId[rolemodel_donorStrategy.getName()].insert(focul_i);

        strategyName2RecipientId[recipients[focul_i].getStrategy().getName()]
            .erase(focul_i);
        recipients[focul_i].setStrategy(rolemodel_recipientStrategy);
        strategyName2RecipientId[rolemodel_recipientStrategy.getName()].insert(
            focul_i);
      }
    }


    int log_step = 1;
    if (stepNum > 100000) {
      log_step = stepNum / 100000;
    }
    if (stepNum > 100000) {
      log_step = stepNum / 100000;
    }
    if (step % log_step == 0) {
      // 生成log
      out.print("{}\n",
                printStatistics(donors, recipients, donorStrategies,
                                recipientStrategies, strategyName2DonorId,
                                strategyName2RecipientId, population, step + 1,
                                false));
    }
  }

  // printStatistics(donors, recipients, donorStrategies, recipientStrategies,
  //                 strategyName2DonorId, strategyName2RecipientId, population,
  //                 stepNum, false);
}

int main() {
// judge if NDEBUG is defined. If defined, the assert() will not work
#ifdef NDEBUG
  std::cout << "NDEBUG is defined\n";
#else
  std::cout << "NDEBUG is not defined\n";
#endif

  std::cout << "Current path is " << std::filesystem::current_path() << '\n';

  // record the running time
  system_clock::time_point start = chrono::system_clock::now();
  tbb::task_arena arena(11);

  // // 硬编码16条进度条 TODO: 由于progressbar 采用单例模式，无法用vector管理
  // dynamicprogressbar可以加入变长的progressbar引用
  // 可以尝试用宏来帮助快速构建多个进度条
  CREATE_BAR(0);
  CREATE_BAR(1);
  CREATE_BAR(2);
  CREATE_BAR(3);
  CREATE_BAR(4);
  CREATE_BAR(5);
  CREATE_BAR(6);
  CREATE_BAR(7);

  // DynamicProgress<ProgressBar> bars(bar0, bar1, bar2, bar3, bar4, bar5, bar6,
  //                                   bar7);

  CREATE_BAR(8);
  CREATE_BAR(9);
  CREATE_BAR(10);
  CREATE_BAR(11);
  CREATE_BAR(12);
  CREATE_BAR(13);
  CREATE_BAR(14);
  CREATE_BAR(15);
  DynamicProgress<ProgressBar> bars(bar0, bar1, bar2, bar3, bar4, bar5, bar6,
                                    bar7, bar8, bar9, bar10, bar11, bar12,
                                    bar13, bar14, bar15);

  // 博弈参数
  int stepNum = 100;
  int population = 160;  // 由于初始化的时候采用了每个策略对相同数量的设置，因此population必须是 4 * 4 的倍数

  if (population % 16 != 0) {
    cerr << "population must be a multiple of 16" << endl;
    return 0;
  }

  double s = 1;          // 费米函数参数

  int b = 4;      // 公共参数
  int beta = 3;   // 公共参数
  int c = 1;      // 公共参数
  int gamma = 1;  // 公共参数
  // double mu = 0.05;  // 动作突变率
  double mu = 0.1;
  int normId = 10;  // 均衡为 d-nr
  double p0 = 1;
  // int normId = 9;
  int updateStepNum = 1;  // 表示每隔多少步更新一次策略

  show_console_cursor(false);

  // // 调试使用
  // CREATE_BAR(100);
  // ProgressBar *bar100_ptr = &bar100;
  // func(stepNum, population, s, b, beta, c, gamma, mu, normId, updateStepNum,
  // p0, bar100_ptr, true);
  // // func(stepNum, population, s, b, beta, c, gamma, mu, normId, updateStepNum);

  // 多线程加速

  arena.execute([&]() {
    int start = 1000000;
    int end = 1000012;
    tbb::parallel_for(start, end, [&](int stepNum) {
      func(stepNum, population, s, b, beta, c, gamma, mu, normId, updateStepNum,
           p0, nullptr, false, &bars, true, stepNum - start);
    });

      // //   //   // tbb::parallel_for(0, 16, [&](int normId){
      // //   //   //   func(stepNum, population, s, b, beta, c, gamma, mu,
      // normId,
      // //   //   //   updateStepNum);
      // //   //   // });
  });

  show_console_cursor(true);
  system_clock::time_point end = system_clock::now();
  cout << "\ntime: " << duration_cast<microseconds>(end - start).count() / 1e6
       << "s" << endl;
  return 0;
}