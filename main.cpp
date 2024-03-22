/**
 * @file main.cpp
 * @author 
 * @brief the simulation of the evolution of cooperation based on the static
 * payoff matrix (combined the reputation and norm)
 *
 * This is a conventional fermi update evolution model, the payoff matrix is
 * considered under the circumstance of long term
 *
 * cite:
 * 1. Pal, S., Hilbe, C., 2022. Reputation effects drive the joint evolution of
 * cooperation and social rewarding. Nat. Commun. 13, 5928.
 * https://doi.org/10.1038/s41467-022-33551-y
 * 2. Guo, H., Song, Z., Geček, S., Li, X., Jusup, M., Perc, M., Moreno, Y.,
 * Boccaletti, S., Wang, Z., 2020. A novel route to cyclic dominance in
 * voluntary social dilemmas. Journal of The Royal Society Interface 17,
 * 20190789. https://doi.org/10.1098/rsif.2019.0789
 *
 * but we have a change. To characterize the dynamic of reputation of the
 * recipient, we should make the game actually played after the imitation done.
 * which means little change of reputation distribution will take place in each
 * step with strategy distribution changed.
 * 1. Focal player imitated the rolemodel considered that the rolemodel will
 * have a higher payoff than the focal player.
 * 2. Focal player immediately play the game with a random select neighbor using
 * the new strategy.
 * 3. The reputation distribution will be updated according to the game result.
 * 4. During the next step, a new focal player will imitate a new rolemodel
 * under the new reputation distribution.
 *
 * It is should be noted that the reputation distribution's dynamic just
 * influence the players only thinking in short term, instead of the players who
 * have a long term thinking(They only consider the stability of the reputation
 * distribution).
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
 * @brief fermi function, which is used to calculate the probability of transition of strategy
 *
 * @param payoff_current the payoff of the current strategy
 * @param payoff_new the payoff of the new strategy
 * @param s the sensitivity of the fermi function if s is large, the probability of transition is small
 * @return double
 */
double fermi(double payoff_current, double payoff_new, double s) {
  double res = 1 / (1 + exp((payoff_current - payoff_new) * s));
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
    const int population,
    const unordered_map<string, set<int>>& reputation2Id, vector<Player>& donors, vector<Player>& recipients) {
  double temp_sum = 0;
  const int n = population;
  int good_rep_num = reputation2Id.at("1").size();

  // for (auto& [do_stg_name, donor_id_set] : strategyName2donorId) {
  //   if (do_stg_name == "C") {
  //     temp_sum += donor_id_set.size() * (n - 1);
  //   } else if (do_stg_name == "D") {
  //     // do nothing
  //   } else if (do_stg_name == "DISC") {
  //     // record the number of people with reputation 1 in disc
  //     int good_rep_in_disc = 0;
  //     for (const auto& id: donor_id_set) {
  //       if (recipients[id].getVarValue(REPUTATION_STR) == 1.0) {
  //         good_rep_in_disc++;
  //       }
  //     }
  //     // the good rep in disc will not encounter themselves, so they cooperate with good_rep_num - 1 people. The bad rep in disc encountering good_rep_num people will cooperate.
  //     temp_sum += good_rep_in_disc * (good_rep_num - 1) + (donor_id_set.size() - good_rep_in_disc) * good_rep_num;
  //   } else if (do_stg_name == "ADISC") {
  //     // record the number of people with reputation 0 in disc
  //     // TODO: ADISC / adisc
  //     int bad_rep_in_ndisc = 0;
  //     for (const auto& id: donor_id_set) {
  //       if (recipients[id].getVarValue(REPUTATION_STR) == 0.0) {
  //         bad_rep_in_ndisc++;
  //       }
  //     }
  //     temp_sum += bad_rep_in_ndisc * (n - good_rep_num - 1) + (donor_id_set.size() - bad_rep_in_ndisc) * (n - good_rep_num);
  //   } else {
  //     cerr << "not support strategy name: " << do_stg_name << endl;
  //     throw "not support strategy name";
  //   }
  // }
  // double res = temp_sum / (n * (n - 1));
  // assert(res >= 0 && res <= 1);

  // play the game for coop_game_times times, and count the number of times the donor cooperates
  // TODO: there are some not good random number generator rand(), it should be replaced by the c++11 random number generator
  int game_times = 1000;
  int coop_times = 0;
  for(int i = 0; i < game_times; i++) {
    int donor_id = rand() % n;
    int recipient_id = rand() % n;
    while(donor_id == recipient_id) {
      recipient_id = rand() % n;
    }
    Action const &donor_action = donors.at(donor_id).donate(to_string((int)recipients.at(recipient_id).getVarValue(REPUTATION_STR)), 0.0);
    Action const &recipient_action = recipients.at(recipient_id).reward(donor_action.getName(), 0.0);
    if (recipient_action.getName() == "C" && donor_action.getName() == "C") {
      coop_times++;
    }
  }
  double res = static_cast<double>(coop_times) / game_times;
  return res;
}

/**
 * @brief Get the Avg Payoff object
 *
 * @param donorStrategy
 * @param recipientStrategy
 * @param payoff_matrix
 * @param strategyName2donorId
 * @param strategyName2recipientId
 * @param population
 * @return double
 */
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
 * @brief Counting the number of people in each policy pair can generate log rows:
 * statistics: C-NR, C-SR, C-AR, C-UR, DISC-NR, DISC-SR, DISC-AR, DISC-UR,
 * step, C-NR, C-SR, C-AR, C-UR, DISC-NR, DISC-SR, DISC-AR, DISC-UR, ADISC-NR,
 * ADISC-SR, ADISC-AR, ADISC-UR, D-NR, D-SR, D-AR, D-UR, C, DISC, ADISC, D, NR,
 * SR, AR, UR, cr
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
 * @param good_rep_num 
 * @return string 
 */
string printStatistics(
    vector<Player>& donors, vector<Player>& recipients,
    const vector<Strategy>& donorStrategies,
    const vector<Strategy>& recipientStrategies,
    const unordered_map<string, set<int>>& strategyName2DonorId,
    const unordered_map<string, set<int>>& strategyName2RecipientId,
    int population, int step, bool print, int good_rep_num) {
  double population_double = static_cast<double>(population);
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
  assert(reputation2Id["1"].size() == good_rep_num);
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

    logLine += "," + to_string(reputation2Id["1"].size() / population_double);
    double coop_rate =
        getCoopRate(strategyName2DonorId, strategyName2RecipientId, population,
                    reputation2Id, donors, recipients);
    logLine += "," + to_string(coop_rate);
  }
  return logLine;
}

/**
 * @brief evolution process
 *
 * @param step_num
 * @param population
 * @param s
 * @param b
 * @param beta
 * @param c
 * @param gamma
 * @param mu
 * @param normId
 * @param update_step_num
 * @param p0
 * @param payoff_matrix_config_name
 * @param bar
 * @param turn_up_progress_bar
 * @param dynamic_bar
 * @param turn_up_dynamic_bar
 * @param dynamic_bar_id
 * @param log_step
 */
void func(int step_num, int population, double s, double b, double beta, double c,
          double gamma, double mu, int norm_id, int update_step_num, double p0,
          string payoff_matrix_config_name, ProgressBar* bar = nullptr,
          bool turn_up_progress_bar = false,
          DynamicProgress<ProgressBar>* dynamic_bar = nullptr,
          bool turn_up_dynamic_bar = false, int dynamic_bar_id = 0,
          int log_step = 1) {
  string norm_name = "norm" + to_string(norm_id);

  PayoffMatrix payoff_matrix("./payoffMatrix/" + payoff_matrix_config_name +
                            "/" + "PayoffMatrix" + to_string(norm_id) + ".csv");

  // assign vars
  payoff_matrix.updateVar("b", b);
  payoff_matrix.updateVar("beta", beta);
  payoff_matrix.updateVar("c", c);
  payoff_matrix.updateVar("gamma", gamma);
  payoff_matrix.updateVar("p", p0);

  payoff_matrix.evalPayoffMatrix();  // TODO: remove this line

  // initialize two players
  vector<Action> donor_actions;
  donor_actions.push_back(Action("C", 0));
  donor_actions.push_back(Action("D", 1));
  // Player donor_temp("donor", 0, donor_actions);
  Player donor_temp("donor", 0, donor_actions);
  vector<Strategy> donor_strategies = payoff_matrix.getRowStrategies();
  donor_temp.setStrategies(donor_strategies);
  donor_temp.loadStrategy("./strategy");
  // fmt::print("donor_strategies: {}\n", donor_temp.getStrategyTables());
  donor_temp.setStrategy("C");

  vector<Action> recipient_actions;
  recipient_actions.push_back(Action("C", 0));
  recipient_actions.push_back(Action("D", 1));
  Player recipient_temp("recipient", 0, recipient_actions);
  vector<Strategy> recipient_strategies = payoff_matrix.getColStrategies();
  recipient_temp.setStrategies(recipient_strategies);

  recipient_temp.loadStrategy("./strategy");
  // fmt::print("recipient_strategies: {}\n",
  // recipient_temp.getStrategyTables());
  recipient_temp.setStrategy("NR");

  // initialize recipient_temp's reputation
  recipient_temp.addVar(REPUTATION_STR, 1);
  // fmt::print("recipient_temp vars: {}\n", recipient_temp.getVars());

  // load norm
  string norm_path = "./norm/" + norm_name + ".csv";
  Norm norm(norm_path);
  // fmt::print("norm: {}\n", norm.getNormTableStr());

  // the two players are used as templates to generate population players
  vector<Player> donors;
  vector<Player> recipients;
  // time seed
  unsigned seed_don = chrono::system_clock::now().time_since_epoch().count();
  default_random_engine gen_don(seed_don);
  uniform_int_distribution<int> dis_don(0, donor_strategies.size() - 1);

  unsigned seed_rec =
      chrono::system_clock::now().time_since_epoch().count() + 1;
  default_random_engine gen_rec(seed_rec);
  uniform_int_distribution<int> dis_rec(0, recipient_strategies.size() - 1);

  unsigned seed_reputation =
      chrono::system_clock::now().time_since_epoch().count() + 2;
  default_random_engine gen_reputation(seed_reputation);
  uniform_int_distribution<int> dis_reputation(0, 1);

  unsigned seed_probability =
      chrono::system_clock::now().time_since_epoch().count() + 3;
  std::mt19937 gen_probability(seed_probability);
  uniform_real_distribution<double> dis_probability(0, 1);

  // record the strategy distribution of the population
  unordered_map<string, set<int>> strategy_name2donor_id;
  unordered_map<string, set<int>> strategy_name2recipient_id;
  // judge if population can be divided by donor_strategies.size()
  assert(population % donor_strategies.size() == 0);

  // reputation init vector
  int good_rep_num = static_cast<int>(population * p0);
  int bad_rep_num = population - good_rep_num;
  vector<int> reputation_value(bad_rep_num, 0);
  vector<int> good_rep_value(good_rep_num, 1);
  assert(reputation_value.size() + good_rep_value.size() == population);

  reputation_value.insert(reputation_value.end(), good_rep_value.begin(),
                          good_rep_value.end());
  random_shuffle(reputation_value.begin(), reputation_value.end());

  // strategy init vector, each strategy pair has the same number of players
  vector<int> donor_stra_id;
  vector<int> recipient_stra_id;
  for (int i = 0; i < donor_strategies.size(); i++) {
    donor_stra_id.insert(
        donor_stra_id.end(),
        population / static_cast<double>(donor_strategies.size()), i);
    for (int j = 0; j < recipient_strategies.size(); j++) {
      recipient_stra_id.insert(
          recipient_stra_id.end(),
          (population / static_cast<double>(donor_strategies.size())) /
              static_cast<double>(recipient_strategies.size()),
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

    temp_donor.setStrategy(donor_strategies[donor_stra_i]);
    strategy_name2donor_id[temp_donor.getStrategy().getName()].insert(i);
    donors.push_back(temp_donor);

    temp_recipient.setStrategy(recipient_strategies[recipient_stra_i]);
    temp_recipient.updateVar(REPUTATION_STR, reputation_value[i]);

    strategy_name2recipient_id[temp_recipient.getStrategy().getName()].insert(i);
    recipients.push_back(temp_recipient);
  }

  // log
  string log_dir = "./log";
  // judge if the path exists, if not, create it
  if (!filesystem::exists(log_dir)) {
    filesystem::create_directory(log_dir);
  }

  const json::value jv = {{"stepNum", step_num},
                          {"population", population},
                          {"s", s},
                          {"b", b},
                          {"beta", beta},
                          {"c", c},
                          {"gamma", gamma},
                          {"mu", mu},
                          {"normId", norm_id},
                          {"p0", p0},
                          {"payoffMatrix", payoff_matrix_config_name},
                          // not model parameters
                          {"other",
                           {
                               {"updateStepNum", update_step_num},
                               {"logStep", log_step},
                           }}};

  string log_file_path = logJson(log_dir, jv);

  auto out = fmt::output_file(log_file_path);
  // generate header
  string line = "step";
  for (Strategy donor_s : donor_strategies) {
    for (Strategy recipient_s : recipient_strategies) {
      line += "," + donor_s.getName() + "-" + recipient_s.getName();
    }
  }
  for (Strategy donor_s : donor_strategies) {
    line += "," + donor_s.getName();
  }
  for (Strategy recipient_s : recipient_strategies) {
    line += "," + recipient_s.getName();
  }
  line += ",good_rep,cr";

  out.print("{}\n", line);

  string log_line =
      printStatistics(donors, recipients, donor_strategies, recipient_strategies,
                      strategy_name2donor_id, strategy_name2recipient_id,
                      population, 0, false, good_rep_num);
  out.print("{}\n", log_line);

  uniform_int_distribution<int> dis(0, population - 1);
  for (int step = 0; step < step_num; step++) {
    // update progress bar
    if (turn_up_progress_bar) {
      // (*bar).set_progress((double)step / step_num * 100);
      // only update once when the progress bar increases by 1%
      if (step % (step_num / 100) == 0) {
        (*bar).tick();
      }
    } else if (turn_up_dynamic_bar) {
      if (step % (step_num / 100) == 0) {
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
    // to prevent the same person from being  drawn
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
        randId_d = donors[focal_i].getRandomInt(0, donor_strategies.size() - 1);
        randId_r =
            recipients[focal_i].getRandomInt(0, recipient_strategies.size() - 1);
      } while (randId_d == donors[focal_i].getStrategy().getId() &&
               randId_r == recipients[focal_i].getStrategy().getId());

      strategy_name2donor_id[donors[focal_i].getStrategy().getName()].erase(
          focal_i);
      donors[focal_i].setStrategy(donor_strategies[randId_d]);
      strategy_name2donor_id[donor_strategies[randId_d].getName()].insert(focal_i);

      strategy_name2recipient_id[recipients[focal_i].getStrategy().getName()]
          .erase(focal_i);
      recipients[focal_i].setStrategy(recipient_strategies[randId_r]);
      strategy_name2recipient_id[recipient_strategies[randId_r].getName()].insert(
          focal_i);
    } else {
      Strategy rolemodel_donorStrategy = donors[rolemodel_i].getStrategy();
      Strategy rolemodel_recipientStrategy =
          recipients[rolemodel_i].getStrategy();

      // if payoff_matrix_config_name == "payoffMatrix_shortterm", then eval the
      // whole payoff_matrix according to the current reputation distribution
      map<string, double> vars_for_recipient = {
          {"p", recipients[rolemodel_i].getVarValue(REPUTATION_STR)}};
      if (payoff_matrix_config_name == "payoffMatrix_shortterm") {
        payoff_matrix.updateVar("p",
                               static_cast<double>(good_rep_num) / population);
        payoff_matrix.evalPayoffMatrix({}, vars_for_recipient);
      } else if (payoff_matrix_config_name ==
                 "payoffMatrix_longterm_no_norm_error") {
        payoff_matrix.evalPayoffMatrix({}, vars_for_recipient);
      } else {
        cerr << "payoff_matrix_config_name error: "
             << payoff_matrix_config_name << endl;
             throw "payoff_matrix_config_name error";
      }

      double rolemodel_payoff = getAvgPayoff(
          rolemodel_donorStrategy, rolemodel_recipientStrategy, payoff_matrix,
          strategy_name2donor_id, strategy_name2recipient_id, population);

      Strategy focul_donorStrategy = donors[focal_i].getStrategy();
      Strategy focul_recipientStrategy = recipients[focal_i].getStrategy();

      // TODO: optimize?
      vars_for_recipient = {
          {"p", recipients[focal_i].getVarValue(REPUTATION_STR)}};
      if (payoff_matrix_config_name == "payoffMatrix_shortterm") {
        payoff_matrix.updateVar("p",
                               static_cast<double>(good_rep_num) / population);
        payoff_matrix.evalPayoffMatrix({}, vars_for_recipient);
      } else if (payoff_matrix_config_name ==
                 "payoffMatrix_longterm_no_norm_error") {
        payoff_matrix.evalPayoffMatrix({}, vars_for_recipient);
      } else {
        cerr << "payoff_matrix_config_name error: "
             << payoff_matrix_config_name << endl;
             throw "payoff_matrix_config_name error";
      }

      double focul_payoff = getAvgPayoff(
          focul_donorStrategy, focul_recipientStrategy, payoff_matrix,
          strategy_name2donor_id, strategy_name2recipient_id, population);

      // fermi
      if (dis_probability(gen_probability) <
          fermi(focul_payoff, rolemodel_payoff, s)) {
        strategy_name2donor_id[donors[focal_i].getStrategy().getName()].erase(
            focal_i);
        donors[focal_i].setStrategy(rolemodel_donorStrategy);
        strategy_name2donor_id[rolemodel_donorStrategy.getName()].insert(focal_i);

        strategy_name2recipient_id[recipients[focal_i].getStrategy().getName()]
            .erase(focal_i);
        recipients[focal_i].setStrategy(rolemodel_recipientStrategy);
        strategy_name2recipient_id[rolemodel_recipientStrategy.getName()].insert(
            focal_i);
      }
    }

    // focal player play the game with a random select neighbor k using the new
    // strategy
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
    Action donor_action = donor->donate(to_string((int)reputation), 0.0);
    Action recipient_action = recipient->reward(donor_action.getName(), 0.0);
    double new_reputation =
        norm.getReputation(donor_action, recipient_action, 0.0);
    recipient->updateVar(REPUTATION_STR, new_reputation);
    if (reputation != new_reputation) {
      if (reputation == 0.0) {
        // good -> bad
        good_rep_num++;
      } else if (reputation == 1.0) {
        // bad -> good
        good_rep_num--;
      } else {
        cerr << "reputation value error: " << reputation << endl;
        throw "reputation value error";
      }
    }

    if (step % log_step == 0) {
      // generate log
      out.print("{}\n",
                printStatistics(donors, recipients, donor_strategies,
                                recipient_strategies, strategy_name2donor_id,
                                strategy_name2recipient_id, population, step + 1,
                                false, good_rep_num));
    }
  }
}

DEFINE_int32(stepNum, 1000, "the number of steps");
DEFINE_int32(population, 160, "the number of population");
DEFINE_double(s, 1, "the parameter of fermi function");
DEFINE_double(b, 4, "the parameter of payoff matrix");
DEFINE_double(beta, 3, "the parameter of payoff matrix");
DEFINE_double(c, 1, "the parameter of payoff matrix");
DEFINE_double(gamma, 1, "the parameter of payoff matrix");
DEFINE_double(mu, 0.0001, "the probability of mutation");
// DEFINE_int32(normId, 10, "the id of norm");
DEFINE_int32(updateStepNum, 1, "the number of steps to update strategy");
DEFINE_double(p0, 1, "the probability of good reputation");
DEFINE_int32(logStep, 1, "the number of steps to log");
DEFINE_int32(threads, 11, "the number of threads");
DEFINE_string(payoff_matrix_config_name, "payoffMatrix_longterm_no_norm_error",
              "the name of payoff matrix config");
// the [start_norm_id, end_norm_id) will be simulated
DEFINE_int32(start_norm_id, 0, "the start norm id");
DEFINE_int32(end_norm_id, 16, "the end norm id");

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

  // Because the same number of strategy pairs is set during initialization,
  // population must be a multiple of 16 (4 * 4)
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

    tbb::parallel_for(FLAGS_start_norm_id, FLAGS_end_norm_id, [&](int normId) {
      func(FLAGS_stepNum, FLAGS_population, FLAGS_s, FLAGS_b, FLAGS_beta,
           FLAGS_c, FLAGS_gamma, FLAGS_mu, normId, FLAGS_updateStepNum,
           FLAGS_p0, FLAGS_payoff_matrix_config_name, nullptr, false, &bars,
           true, normId, FLAGS_logStep);
    });
  });

  show_console_cursor(true);
  system_clock::time_point end = system_clock::now();
  cout << "\ntime: " << duration_cast<microseconds>(end - start).count() / 1e6
       << "s" << endl;
  return 0;
}