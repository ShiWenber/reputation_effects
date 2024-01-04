/**
 * @file main.cpp
 * @author ShiWenber (1210169842@qq.com)
 * @brief  the simulation of the evolution of cooperation based on q-learning
 * @version 0.1
 * @date 2023-12-25
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <cmath>
#include <iostream>
// 导入字典类型
#include <fmt/os.h>
#include <fmt/ranges.h>
#include <gflags/gflags.h>

#include <RewardMatrix.hpp>
#include <cassert>
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

#include <boost/json.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <indicators/cursor_control.hpp>
#include <indicators/dynamic_progress.hpp>
#include <indicators/progress_bar.hpp>
#include <numeric>

#include "Action.hpp"
#include "Buffer.hpp"
#include "JsonFile.hpp"
#include "Norm.hpp"
#include "Player.hpp"
#include "RewardMatrix.hpp"
#include "Transition.hpp"

#define REPUTATION_STR "reputation"
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

using namespace std;
using namespace indicators;
using namespace std::chrono;
using namespace boost;

/**
 * @brief fermi function, used to calculate the probability of strategy change
 *
 * @param payoff_current 0.5 * (donor + recipient) the expected payoff of the
 * focal player
 * @param payoff_new 0.5 * (donor + recipient) the expected payoff of the role
 * model player
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
 * @brief Get the Coop Rate object,
 * we randomly select two people from the population to play the game. Because
 * there are identity differences between the two people, it is ordered, and
 * there are A_n^2 = n * (n - 1) possible
 *
 * Among all these possible extractions, the number of times the *donor*
 * cooperate is used as the numerator
 *
 * so the cooperation rate is possibleCoopNum / (n * (n - 1))
 *
 * This only reflects the probability of the donor to make a donation, and does
 * not reflect the probability of the recipient to give feedback
 *
 * @param strategyPair2Num
 * @param population
 * @return double
 */
double getCoopRate(
    const unordered_map<string, set<int>>& strategyName2donorId,
    const unordered_map<string, set<int>>& strategyName2recipientId,
    const int& population,
    const unordered_map<string, set<int>>& reputation2Id) {
  double temp_sum = 0;
  const int& n = population;
  int good_rep_num = reputation2Id.at("1").size();

  for (auto& [do_stg_name, donorIdSet] : strategyName2donorId) {
    if (do_stg_name == "C") {
      temp_sum += donorIdSet.size() * (n - 1);
    } else if (do_stg_name == "D") {
      // do nothing
    } else if (do_stg_name == "DISC") {
      // 如果好人中有DISC存在，那么乘 good_rep_num - 1 否则 乘 good_rep_num
      //// 对集合 reputation2Id["1"] 和 donorIdSet 求交集如果数量 > 0，那么乘
      /// good_rep_num - 1

      temp_sum +=
          donorIdSet.size() * good_rep_num;  // error, should reduce the number
                                             // of "DISC" in good_rep_num
    } else if (do_stg_name == "NDISC") {
      // temp_sum += strategyName2donorId.at(do_stg_name).size() * (n - 1);
      temp_sum += donorIdSet.size() * (n - good_rep_num);
    } else {
      cerr << "not support strategy name: " << do_stg_name << endl;
      throw "not support strategy name";
    }

    for (auto& [strategyName, recipientIdSet] : strategyName2recipientId) {
      for (auto& donorId : donorIdSet) {
        for (auto& recipientId : recipientIdSet) {
          if (donorId != recipientId) {
            temp_sum += 1;
          }
        }
      }
    }
  }

  return temp_sum / (n * (n - 1));
}

/**
 * @brief  统计每个策略对的人数同时可以生成log行:
 * step, C-NR, C-SR, C-AR, C-UR, DISC-NR, DISC-SR, DISC-AR, DISC-UR, NDISC-NR,
 * NDISC-SR, NDISC-AR, NDISC-UR, D-NR, D-SR, D-AR, D-UR, C, DISC, NDISC, D, NR,
 * SR, AR, UR, cr
 *
 *
 * @param donors
 * @param recipients
 * @param donorStrategies
 * @param recipientStrategies
 * @param i_e
 * @param good_rep_num
 * @param step_num
 * @param cooperate_times
 * @return string
 */
string printStatistics(vector<Player>& donors, vector<Player>& recipients,
                       const vector<Strategy>& donorStrategies,
                       const vector<Strategy>& recipientStrategies, int i_e,
                       int good_rep_num, int step_num, double cooperate_times,
                       double reward_two_players, int population) {
  string log_line = to_string(i_e);

  double population_double = static_cast<double>(population);
  // unordered_map<string, int> strategyPair2Num;
  unordered_map<string, int> strategyPair2Num;
  string key_str;
  unordered_map<string, set<int>> reputation2Id;
  for (int i = 0; i < population; i++) {
    Player& donor = donors[i];
    Player& recipient = recipients[i];
    key_str = donor.getStrategyNameFromQTable(0) + "-" +
              recipient.getStrategyNameFromQTable(1);
    strategyPair2Num[key_str]++;
  }

  for (Strategy donorS : donorStrategies) {
    for (Strategy recipientS : recipientStrategies) {
      key_str = donorS.getName() + "-" + recipientS.getName();
      log_line +=
          "," + to_string(strategyPair2Num[key_str] / population_double);
    }
  }
  log_line += "," + to_string(good_rep_num / static_cast<double>(population));
  log_line += "," + to_string(cooperate_times / static_cast<double>(step_num));
  log_line +=
      "," + to_string(reward_two_players / static_cast<double>(step_num));
  return log_line;
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
void func(int stepNum, int episode, int buffer_capacity, int batch_size,
          double alpha, double discount, int population, double s, int b,
          int beta, int c, int gamma, double mu, double epsilon,
          double beta_boltzmann, bool with_boltzmann, int normId,
          int updateStepNum, double p0, string payoff_matrix_config_name,
          ProgressBar* bar = nullptr, bool turn_up_progress_bar = false,
          DynamicProgress<ProgressBar>* dynamic_bar = nullptr,
          bool turn_up_dynamic_bar = false, int dynamic_bar_id = 0,
          int log_step = 1) {
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

  RewardMatrix rewardMatrix("./rewardMatrix/RewardMatrix.csv");

  // assign vars
  rewardMatrix.updateVar("b", b);
  rewardMatrix.updateVar("beta", beta);
  rewardMatrix.updateVar("c", c);
  rewardMatrix.updateVar("gamma", gamma);
  rewardMatrix.updateVar("p", p0);

  rewardMatrix.evalRewardMatrix();
  // rewardMatrix.print();

  // initialize two players
  vector<Strategy> donorStrategies;
  donorStrategies.push_back(Strategy("C", 0));
  donorStrategies.push_back(Strategy("DISC", 1));
  donorStrategies.push_back(Strategy("NDISC", 2));
  donorStrategies.push_back(Strategy("D", 3));

  vector<Strategy> recipientStrategies;
  recipientStrategies.push_back(Strategy("NR", 0));
  recipientStrategies.push_back(Strategy("SR", 1));
  recipientStrategies.push_back(Strategy("AR", 2));
  recipientStrategies.push_back(Strategy("UR", 3));

  vector<Action> donorActions;
  donorActions.push_back(Action("C", 0));
  donorActions.push_back(Action("D", 1));
  Player donor_temp("donor", 0, donorActions, {"0", "1"}, {"C", "D"});

  vector<Action> recipientActions;
  recipientActions.push_back(Action("C", 0));
  recipientActions.push_back(Action("D", 1));
  Player recipient_temp("recipient", 0, recipientActions, {"C", "D"},
                        {"C", "D"});
  recipient_temp.addVar(REPUTATION_STR, 0);

  // load norm
  string normPath = "./norm/" + normName + ".csv";
  Norm norm(normPath);

  // the two players are used as templates to generate population players
  vector<Player> donors;
  vector<Player> recipients;

  // use different time seeds to generate random numbers
  unsigned seed_don =
      chrono::system_clock::now()
          .time_since_epoch()
          .count();  // add the specified value to avoid the same seed
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
  std::mt19937 gen_probability(seed_probability);
  uniform_real_distribution<double> dis_probability(0, 1);

  // reputation init vector
  int good_rep_num = (int)population * p0;
  vector<int> reputation_value(ceil(population * (1 - p0)), 0);
  vector<int> good_rep_value((int)population * p0, 1);
  assert(reputation_value.size() + good_rep_value.size() == population);

  reputation_value.insert(reputation_value.end(), good_rep_value.begin(),
                          good_rep_value.end());
  random_shuffle(reputation_value.begin(), reputation_value.end());

  // initialize population players that have different strategies, and each
  // strategy has the same number of players
  for (int i = 0; i < population; i++) {
    Player temp_donor(donor_temp);
    Player temp_recipient(recipient_temp);
    temp_recipient.updateVar(REPUTATION_STR, reputation_value[i]);

    donors.push_back(temp_donor);
    recipients.push_back(temp_recipient);
  }

  // log
  string log_dir = "./log";
  // judge if the path exists, if not, create it
  if (!filesystem::exists(log_dir)) {
    filesystem::create_directory(log_dir);
  }

  const json::value jv = {{"stepNum", stepNum},
                          {"episode", episode},
                          {"buffer_capacity", buffer_capacity},
                          {"population", population},
                          {"s", s},
                          {"b", b},
                          {"beta", beta},
                          {"c", c},
                          {"gamma", gamma},
                          {"mu", mu},
                          {"normId", normId},
                          {"p0", p0},
                          {"payoffMatrix", payoff_matrix_config_name},
                          {"epsilon", epsilon},
                          {"batch_size", batch_size},
                          {"alpha", alpha},
                          {"discount", discount},
                          // q-learning with boltzmann
                          {"with_boltzmann", with_boltzmann},
                          {"beta_boltzmann", beta_boltzmann},
                          // not model parameters
                          {"other",
                           {
                               {"updateStepNum", updateStepNum},
                               {"logStep", log_step},
                           }}};

  string log_file_path = logJson(log_dir, jv);

  auto out = fmt::output_file(log_file_path);

  // log header
  string log_header = "episode";
  for (Strategy donorS : donorStrategies) {
    for (Strategy recipientS : recipientStrategies) {
      log_header += "," + donorS.getName() + "-" + recipientS.getName();
    }
  }
  log_header += ",good_rep,cr,reward";
  out.print("{}\n", log_header);

  uniform_int_distribution<int> dis(0, population - 1);

  for (int i_e = 0; i_e < episode; i_e++) {
    // update progress bar
    if (turn_up_progress_bar) {
      // (*bar).set_progress((double)step / stepNum * 100);
      // only update once when the progress bar increases by 1%
      if (i_e % (episode / 100) == 0) {
        (*bar).tick();
      }
    } else if (turn_up_dynamic_bar) {
      if (i_e % (episode / 100) == 0) {
        (*dynamic_bar)[dynamic_bar_id].tick();
      }
    }

    // refresh buffer  0 represent the donor, 1 represent the recipient
    Buffer buffer(2, population, buffer_capacity);
    int cooperate_times = 0;
    double reward_two_players =
        0;  // TODO: this is only one step reward computing from the donor's
            // reward + recipient's reward
    for (int step = 0; step < stepNum; step++) {
      // The random number of 0-population is extracted
      // i_1 as donor, i_2 as recipient
      int i_1 = dis(gen_don);
      int i_2 = dis(gen_rec);
      // to prevent the same person from being drawn
      while (i_1 == i_2) {
        i_1 = dis(gen_don);
        i_2 = dis(gen_rec);
      }

      Player& donor = donors[i_1];
      Player& recipient = recipients[i_2];

      double reputation = recipient.getVarValue(REPUTATION_STR);
      Action donor_action =
          donor.donate(to_string((int)reputation), epsilon, beta_boltzmann, 0.0,
                       true, with_boltzmann);
      Action recipient_action =
          recipient.reward(donor_action.getName(), epsilon, beta_boltzmann, 0.0,
                           true, with_boltzmann);
      double new_reputation =
          norm.getReputation(donor_action, recipient_action, 0.0);
      recipient.updateVar(REPUTATION_STR, new_reputation);

      // update the reputation distribution
      if (reputation != new_reputation) {
        if (reputation == 1 && new_reputation == 0) {
          good_rep_num--;
        } else if (reputation == 0 && new_reputation == 1) {
          good_rep_num++;
        } else {
          cerr << "reputation error: " << reputation << "," << new_reputation
               << endl;
          throw "reputation error";
        }
      }

      const vector<double>& rewards =
          rewardMatrix.getReward(donor_action, recipient_action);
      double donor_r = rewards[0];
      double recipient_r = rewards[1];
      reward_two_players += (donor_r + recipient_r);

      // should update the q table of donor? TODO: this is direct reciprocity
      // not the indirect reciprocity
      Action new_donor_action =
          donor.donate(to_string((int)new_reputation), epsilon, beta_boltzmann,
                       0.0, true, with_boltzmann);

      // buffer save the progress of the two players
      Transition do_transition(to_string(static_cast<int>(reputation)),
                               donor_action, donor_r,
                               to_string(static_cast<int>(new_reputation)));
      Transition re_transition(donor_action.getName(), recipient_action,
                               recipient_r, new_donor_action.getName());
      // this is experience for i_1 as donor
      buffer.add(0, i_1, do_transition);
      // this is experience for i_2 as recipient
      buffer.add(1, i_2, re_transition);

      // TODO: just donor C or donor-recipient all C
      if (donor_action.getName() == "C") {
        cooperate_times++;
      }

      if (buffer.size(0, i_1) > batch_size) {
        // sample from the buffer
        vector<Transition> do_batch = buffer.sample(0, i_1, batch_size);
        // update q table
        donor.updateQTable(do_batch, alpha, discount);
      }
      if (buffer.size(1, i_2) > batch_size) {
        vector<Transition> re_batch = buffer.sample(1, i_2, batch_size);
        recipient.updateQTable(re_batch, alpha, discount);
      }
    }
    if (i_e % log_step == 0) {
      const string& log_line =
          printStatistics(donors, recipients, donorStrategies,
                          recipientStrategies, i_e, good_rep_num, stepNum,
                          cooperate_times, reward_two_players, population);
      out.print("{}\n", log_line);
    }
  }
}

DEFINE_int32(stepNum, 200, "the number of steps in each episode");
DEFINE_int32(episode, 20000, "the number of episodes");
DEFINE_int32(buffer_capacity, 1000, "the capacity of buffer");
DEFINE_int32(batch_size, 1, "the size of batch");
DEFINE_double(alpha, 0.1, "the learning rate");
DEFINE_double(discount, 0.99, "the discount factor");
DEFINE_int32(population, 16, "the number of population");
DEFINE_double(s, 1, "the parameter of fermi function");
DEFINE_int32(b, 4, "the parameter of payoff matrix");
DEFINE_int32(beta, 3, "the parameter of payoff matrix");
DEFINE_int32(c, 1, "the parameter of payoff matrix");
DEFINE_int32(gamma, 1, "the parameter of payoff matrix");
DEFINE_double(mu, 0.05, "the probability of mutation");
DEFINE_double(epsilon, 0.1, "the probability of mutation");
// DEFINE_int32(normId, 10, "the id of norm");
DEFINE_int32(updateStepNum, 1, "the number of steps to update strategy");
DEFINE_double(p0, 1, "the probability of good reputation");
DEFINE_int32(logStep, 1, "the number of steps to log");
DEFINE_int32(threads, 8, "the number of threads");
DEFINE_string(payoff_matrix_config_name, "payoffMatrix_longterm_no_norm_error",
              "the name of payoff matrix config");
DEFINE_bool(with_boltzmann, false, "whether use boltzmann distribution");
DEFINE_double(beta_boltzmann, 2, "the parameter of boltzmann distribution");

int main(int argc, char** argv) {
  gflags::SetUsageMessage(
      "the simulation of the evolution of cooperation based on the static "
      "payoff matrix (combined the reputation and norm)");
  gflags::SetVersionString("0.1");
  gflags::ParseCommandLineFlags(&argc, &argv, true);

// judge if NDEBUG is defined. If defined, the assert() will not work
#ifdef NDEBUG
  std::cout << "NDEBUG is defined\n";
#else
  std::cout << "NDEBUG is not defined\n";
#endif

  std::cout << "Current path is " << std::filesystem::current_path() << '\n';
  std::cout << "with_boltzmann: " << FLAGS_with_boltzmann << '\n';

  // record the running time
  system_clock::time_point start = chrono::system_clock::now();
  // conform the number of threads is 1 <= threads <= 16
  if (FLAGS_threads < 1 || FLAGS_threads > 16) {
    cerr << "threads must be 1 <= threads <= 16" << endl;
    return 0;
  }
  tbb::task_arena arena(FLAGS_threads);

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
  // int stepNum = 100000;
  // int population =
  //     160;  //
  //     由于初始化的时候采用了每个策略对相同数量的设置，因此population必须是
  //           // 4 * 4 的倍数

  // if (FLAGS_population % 16 != 0) {
  //   cerr << "population must be a multiple of 16" << endl;
  //   return 0;
  // }

  // double s = 1;  // 费米函数参数

  // int b = 4;      // 公共参数
  // int beta = 3;   // 公共参数
  // int c = 1;      // 公共参数
  // int gamma = 1;  // 公共参数
  // // double mu = 0.05;  // 动作突变率
  // double mu = 0.1;
  // int normId = 10;  // 均衡为 d-nr
  // double p0 = 1;
  // // int normId = 9;
  // int updateStepNum = 1;  // 表示每隔多少步更新一次策略

  show_console_cursor(false);

  // // for debug
  // CREATE_BAR(100);
  // ProgressBar *bar100_ptr = &bar100;
  // func(stepNum, population, s, b, beta, c, gamma, mu, normId, updateStepNum,
  // p0, bar100_ptr, true);
  // // func(stepNum, population, s, b, beta, c, gamma, mu, normId,
  // updateStepNum);

  arena.execute([&]() {
    // int start = 1000000;
    // int end = 1000012;
    // tbb::parallel_for(start, end, [&](int stepNum) {
    //   func(stepNum, population, s, b, beta, c, gamma, mu, normId,
    //   updateStepNum,
    //        p0, nullptr, false, &bars, true, stepNum - start);
    // });

    tbb::parallel_for(0, 16, [&](int normId) {
      func(FLAGS_stepNum, FLAGS_episode, FLAGS_buffer_capacity,
           FLAGS_batch_size, FLAGS_alpha, FLAGS_discount, FLAGS_population,
           FLAGS_s, FLAGS_b, FLAGS_beta, FLAGS_c, FLAGS_gamma, FLAGS_mu,
           FLAGS_epsilon, FLAGS_beta_boltzmann, FLAGS_with_boltzmann, normId,
           FLAGS_updateStepNum, FLAGS_p0, FLAGS_payoff_matrix_config_name,
           nullptr, false, &bars, true, normId, FLAGS_logStep);
    });
  });

  show_console_cursor(true);
  system_clock::time_point end = system_clock::now();
  cout << "\ntime: " << duration_cast<microseconds>(end - start).count() / 1e6
       << "s" << endl;

  return 0;
}