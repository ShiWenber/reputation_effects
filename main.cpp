/**
 * @file main.cpp
 * @author ShiWenber (1210169842@qq.com)
 * @brief the simulation of the evolution of cooperation based on the static
 * payoff matrix (combined the reputation and norm)
 * 
 * This is a conventional fermi update evolution model, the payoff matrix is considered under the circumstance of long term
 * 
 * cite:
 * 1. Pal, S., Hilbe, C., 2022. Reputation effects drive the joint evolution of cooperation and social rewarding. Nat. Commun. 13, 5928. https://doi.org/10.1038/s41467-022-33551-y
 * 2. Guo, H., Song, Z., Geček, S., Li, X., Jusup, M., Perc, M., Moreno, Y., Boccaletti, S., Wang, Z., 2020. A novel route to cyclic dominance in voluntary social dilemmas. Journal of The Royal Society Interface 17, 20190789. https://doi.org/10.1098/rsif.2019.0789
 * 
 * but we have a change. To characterize the dynamic of reputation of the recipient, we should make the game actually played after the imitation done.
 * which means little change of reputation distribution will take place in each step with strategy distribution changed.
 * 1. Focal player imitated the rolemodel considered that the rolemodel will have a higher payoff than the focal player.
 * 2. Focal player immediately play the game with a random select neighbor using the new strategy.
 * 3. The reputation distribution will be updated according to the game result.
 * 4. During the next step, a new focal player will imitate a new rolemodel under the new reputation distribution.
 * 
 * It is should be noted that the reputation distribution's dynamic just influence the players only thinking in short term, instead of the players who have a long term thinking(They only consider the stability of the reputation distribution).
 * 
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
#include <gflags/gflags.h>

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
#include "JsonFile.hpp"
#include "Norm.hpp"
#include "PayoffMatrix.hpp"
#include "Player.hpp"
#include "Strategy.hpp"

#define REPUTATION_STR "reputation"

using namespace std;
using namespace indicators;
using namespace std::chrono;
using namespace boost;

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
      ///good_rep_num - 1

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

double getAvgPayoff(
    const Strategy& donorStrategy, const Strategy& recipientStrategy,
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
 * step, C-NR, C-SR, C-AR, C-UR, DISC-NR, DISC-SR, DISC-AR, DISC-UR, NDISC-NR,
 * NDISC-SR, NDISC-AR, NDISC-UR, D-NR, D-SR, D-AR, D-UR, C, DISC, NDISC, D, NR,
 * SR, AR, UR, cr
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
string printStatistics(
    const vector<Player>& donors, const vector<Player>& recipients,
    const vector<Strategy>& donorStrategies,
    const vector<Strategy>& recipientStrategies,
    const unordered_map<string, set<int>>& strategyName2DonorId,
    const unordered_map<string, set<int>>& strategyName2RecipientId,
    int population, int step, bool print) {
  double population_double = (double)population;
  // unordered_map<string, int> strategyPair2Num;
  unordered_map<string, int> strategyPair2Num;
  string key_str;
  unordered_map<string, set<int>> reputation2Id;
  for (int i = 0; i < population; i++) {
    const Player& donor = donors[i];
    const Player& recipient = recipients[i];
    key_str =
        donor.getStrategy().getName() + "-" + recipient.getStrategy().getName();
    strategyPair2Num[key_str]++;

    if (recipient.getVarValue(REPUTATION_STR) == 1.0) {
      reputation2Id["1"].insert(i);
    } else if (recipient.getVarValue(REPUTATION_STR) == 0.0) {
      reputation2Id["0"].insert(i);
    } else {
      cerr << "reputation value error: "
           << recipient.getVarValue(REPUTATION_STR) << endl;
      throw "reputation value error";
    }
    
  }
  assert(reputation2Id["1"].size() + reputation2Id["0"].size() == population);

  string logLine = to_string(step);

  if (print) {
    fmt::print("strategyPair2Num: {}\n", strategyPair2Num);
    for (Strategy donorS : donorStrategies) {
      for (Strategy recipientS : recipientStrategies) {
        key_str = donorS.getName() + "-" + recipientS.getName();
        fmt::print("{0}: {1}, ", key_str,
                   strategyPair2Num[key_str] / population_double);
      }
    }
    fmt::print("\n");
    for (Strategy donorS : donorStrategies) {
      key_str = donorS.getName();
      fmt::print("{0}: {1}, ", key_str,
                 strategyName2DonorId.at(key_str).size() / population_double);
    }
    fmt::print("\n");
    for (Strategy recipientS : recipientStrategies) {
      key_str = recipientS.getName();
      fmt::print(
          "{0}: {1}, ", key_str,
          strategyName2RecipientId.at(key_str).size() / population_double);
    }
  } else {
    for (Strategy donorS : donorStrategies) {
      for (Strategy recipientS : recipientStrategies) {
        key_str = donorS.getName() + "-" + recipientS.getName();
        logLine +=
            "," + to_string(strategyPair2Num[key_str] / population_double);
      }
    }
    for (Strategy donorS : donorStrategies) {
      key_str = donorS.getName();
      logLine += "," + to_string(strategyName2DonorId.at(key_str).size() /
                                 population_double);
    }
    for (Strategy recipientS : recipientStrategies) {
      key_str = recipientS.getName();
      logLine += "," + to_string(strategyName2RecipientId.at(key_str).size() /
                                 population_double);
    }

    logLine += "," + to_string(reputation2Id["1"].size());
    double coop_rate =
        getCoopRate(strategyName2DonorId, strategyName2RecipientId, population,
                    reputation2Id);
    logLine += "," + to_string(coop_rate);
  }
  return logLine;
}


/**
 * @brief evolution process
 * 
 * @param stepNum 
 * @param population 
 * @param s 
 * @param b 
 * @param beta 
 * @param c 
 * @param gamma 
 * @param mu 
 * @param normId 
 * @param updateStepNum 
 * @param p0 
 * @param payoff_matrix_config_name 
 * @param bar 
 * @param turn_up_progress_bar 
 * @param dynamic_bar 
 * @param turn_up_dynamic_bar 
 * @param dynamic_bar_id 
 * @param log_step 
 */
void func(int stepNum, int population, double s, int b, int beta, int c,
          int gamma, double mu, int normId, int updateStepNum, double p0, string payoff_matrix_config_name,
          ProgressBar* bar = nullptr, bool turn_up_progress_bar = false,
          DynamicProgress<ProgressBar>* dynamic_bar = nullptr,
          bool turn_up_dynamic_bar = false, int dynamic_bar_id = 0,
          int log_step = 1) {

  string normName = "norm" + to_string(normId);

  PayoffMatrix payoffMatrix("./payoffMatrix/" + payoff_matrix_config_name + "/" + "PayoffMatrix" + to_string(normId) +
                            ".csv");

  // assign vars
  payoffMatrix.updateVar("b", b);
  payoffMatrix.updateVar("beta", beta);
  payoffMatrix.updateVar("c", c);
  payoffMatrix.updateVar("gamma", gamma);
  payoffMatrix.updateVar("p", p0);

  payoffMatrix.evalPayoffMatrix();

  // initialize two players
  vector<Action> donorActions;
  donorActions.push_back(Action("C", 0));
  donorActions.push_back(Action("D", 1));
  Player donor_temp("donor", 0, donorActions);
  vector<Strategy> donorStrategies = payoffMatrix.getRowStrategies();
  donor_temp.setStrategies(donorStrategies);
  donor_temp.loadStrategy("./strategy");
  // fmt::print("donorStrategies: {}\n", donor_temp.getStrategyTables());
  donor_temp.setStrategy("C");

  vector<Action> recipientActions;
  recipientActions.push_back(Action("C", 0));
  recipientActions.push_back(Action("D", 1));
  Player recipient_temp("recipient", 0, recipientActions);
  vector<Strategy> recipientStrategies = payoffMatrix.getColStrategies();
  recipient_temp.setStrategies(recipientStrategies);

  recipient_temp.loadStrategy("./strategy");
  // fmt::print("recipientStrategies: {}\n", recipient_temp.getStrategyTables());
  recipient_temp.setStrategy("NR");

  // initialize recipient_temp's reputation
  recipient_temp.addVar(REPUTATION_STR, 1);
  // fmt::print("recipient_temp vars: {}\n", recipient_temp.getVars());

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
  std::mt19937 gen_probability(seed_probability);
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
    Player temp_donor(donor_temp);
    Player temp_recipient(recipient_temp);

    auto [donor_stra_i, recipient_stra_i] = stra_id_pairs[i];

    temp_donor.setStrategy(donorStrategies[donor_stra_i]);
    strategyName2DonorId[temp_donor.getStrategy().getName()].insert(i);
    donors.push_back(temp_donor);

    temp_recipient.setStrategy(recipientStrategies[recipient_stra_i]);
    temp_recipient.updateVar(REPUTATION_STR, reputation_value[i]);

    strategyName2RecipientId[temp_recipient.getStrategy().getName()].insert(i);
    recipients.push_back(temp_recipient);
  }

  // log
  string log_dir = "./log";
  // judge if the path exists, if not, create it
  if (!filesystem::exists(log_dir)) {
    filesystem::create_directory(log_dir);
  }

  const json::value jv =
  { {"stepNum", stepNum},
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
    // not model parameters
    {"other",
     {
         {"updateStepNum", updateStepNum},
         {"logStep", log_step},
     }
    }
  };

  string log_file_path = logJson(log_dir, jv);

  auto out = fmt::output_file(log_file_path);
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
  line += ",good_rep_num,cr";

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
    int focal_i = dis(gen_don);
    int rolemodel_i = dis(gen_rec);
    // to prevent the same person from being drawn
    while (focal_i == rolemodel_i) {
      focal_i = dis(gen_don);
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
        randId_d = donors[focal_i].getRandomInt(0, donorStrategies.size() - 1);
        randId_r =
            recipients[focal_i].getRandomInt(0, recipientStrategies.size() - 1);
      } while (randId_d == donors[focal_i].getStrategy().getId() &&
               randId_r == recipients[focal_i].getStrategy().getId());

      strategyName2DonorId[donors[focal_i].getStrategy().getName()].erase(
          focal_i);
      donors[focal_i].setStrategy(donorStrategies[randId_d]);
      strategyName2DonorId[donorStrategies[randId_d].getName()].insert(focal_i);

      strategyName2RecipientId[recipients[focal_i].getStrategy().getName()]
          .erase(focal_i);
      recipients[focal_i].setStrategy(recipientStrategies[randId_r]);
      strategyName2RecipientId[recipientStrategies[randId_r].getName()].insert(
          focal_i);
    } else {
      Strategy rolemodel_donorStrategy = donors[rolemodel_i].getStrategy();
      Strategy rolemodel_recipientStrategy =
          recipients[rolemodel_i].getStrategy();
      double rolemodel_payoff = getAvgPayoff(
          rolemodel_donorStrategy, rolemodel_recipientStrategy, payoffMatrix,
          strategyName2DonorId, strategyName2RecipientId, population);

      Strategy focul_donorStrategy = donors[focal_i].getStrategy();
      Strategy focul_recipientStrategy = recipients[focal_i].getStrategy();
      double focul_payoff = getAvgPayoff(
          focul_donorStrategy, focul_recipientStrategy, payoffMatrix,
          strategyName2DonorId, strategyName2RecipientId, population);

      // fermi
      if (dis_probability(gen_probability) <
          fermi(focul_payoff, rolemodel_payoff, s)) {
        strategyName2DonorId[donors[focal_i].getStrategy().getName()].erase(
            focal_i);
        donors[focal_i].setStrategy(rolemodel_donorStrategy);
        strategyName2DonorId[rolemodel_donorStrategy.getName()].insert(focal_i);

        strategyName2RecipientId[recipients[focal_i].getStrategy().getName()]
            .erase(focal_i);
        recipients[focal_i].setStrategy(rolemodel_recipientStrategy);
        strategyName2RecipientId[rolemodel_recipientStrategy.getName()].insert(
            focal_i);
      }
    }

    // focal player play the game with a random select neighbor k using the new strategy
    int k = dis(gen_don);
    while (k == focal_i) {
      k = dis(gen_don);
    }

    // The position in the game is randomly selected between focal_i and k
    // 1. focal_i as donor and k as recipient
    // 2. k as donor and focal_i as recipient
    double random_p = dis_probability(gen_probability);
    assert(random_p >= 0 && random_p <= 1);
    Player* donor;
    Player* recipient;
    if (random_p > 0.5) {
      donor = &donors[focal_i];
      recipient = &recipients[k];
    } else {
      donor = &donors[k];
      recipient = &recipients[focal_i];
    }

    double reputation = recipient->getVarValue(REPUTATION_STR);
    Action donor_action = donor->donate(to_string((int) reputation), 0.0);
    Action recipient_action = recipient->reward(donor_action.getName(), 0.0);
    double new_reputation = norm.getReputation(donor_action, recipient_action, 0.0);
    recipient->updateVar(REPUTATION_STR, new_reputation);

    if (step % log_step == 0) {
      // 生成log
      out.print("{}\n",
                printStatistics(donors, recipients, donorStrategies,
                                recipientStrategies, strategyName2DonorId,
                                strategyName2RecipientId, population, step + 1,
                                false));
    }
  }
  
}

DEFINE_int32(stepNum, 1000, "the number of steps");
DEFINE_int32(population, 160, "the number of population");
DEFINE_double(s, 1, "the parameter of fermi function");
DEFINE_int32(b, 4, "the parameter of payoff matrix");
DEFINE_int32(beta, 3, "the parameter of payoff matrix");
DEFINE_int32(c, 1, "the parameter of payoff matrix");
DEFINE_int32(gamma, 1, "the parameter of payoff matrix");
DEFINE_double(mu, 0.05, "the probability of mutation");
// DEFINE_int32(normId, 10, "the id of norm");
DEFINE_int32(updateStepNum, 1, "the number of steps to update strategy");
DEFINE_double(p0, 1, "the probability of good reputation");
DEFINE_int32(logStep, 1, "the number of steps to log");
DEFINE_int32(threads, 11, "the number of threads");
DEFINE_string(payoff_matrix_config_name, "payoffMatrix_longterm_no_norm_error", "the name of payoff matrix config");

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

  // record the running time
  system_clock::time_point start = chrono::system_clock::now();
  // conform the number of threads is 1 <= threads <= 16
  if (FLAGS_threads < 1 || FLAGS_threads > 16) {
    cerr << "threads must be 1 <= threads <= 16" << endl;
    return 0;
  }
  tbb::task_arena arena(FLAGS_threads);

  // the macro can help to create multiple progress bars quickly
  CREATE_BAR(0);
  CREATE_BAR(1);
  CREATE_BAR(2);
  CREATE_BAR(3);
  CREATE_BAR(4);
  CREATE_BAR(5);
  CREATE_BAR(6);
  CREATE_BAR(7);
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

  // Because the same number of strategy pairs is set during initialization, population must be a multiple of 16 (4 * 4)
  if (FLAGS_population % 16 != 0) {
    cerr << "population must be a multiple of 16" << endl;
    return 0;
  }

  show_console_cursor(false);

  // // for debug
  // CREATE_BAR(100);
  // ProgressBar *bar100_ptr = &bar100;
  // func(stepNum, population, s, b, beta, c, gamma, mu, normId, updateStepNum,
  // p0, bar100_ptr, true);
  // // func(stepNum, population, s, b, beta, c, gamma, mu, normId,
  // updateStepNum);

  // multithread
  arena.execute([&]() {
    // int start = 1000000;
    // int end = 1000012;
    // tbb::parallel_for(start, end, [&](int stepNum) {
    //   func(stepNum, population, s, b, beta, c, gamma, mu, normId,
    //   updateStepNum,
    //        p0, nullptr, false, &bars, true, stepNum - start);
    // });

    tbb::parallel_for(0, 16, [&](int normId) {
      func(FLAGS_stepNum, FLAGS_population, FLAGS_s, FLAGS_b, FLAGS_beta,
           FLAGS_c, FLAGS_gamma, FLAGS_mu, normId, FLAGS_updateStepNum,
           FLAGS_p0, FLAGS_payoff_matrix_config_name,
             nullptr, false, &bars, true, normId, FLAGS_logStep);
    });
  });

  show_console_cursor(true);
  system_clock::time_point end = system_clock::now();
  cout << "\ntime: " << duration_cast<microseconds>(end - start).count() / 1e6
       << "s" << endl;
  return 0;
}