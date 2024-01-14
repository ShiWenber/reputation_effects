#include "Player.hpp"

#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>

#include "QTable.hpp"

// init static member commonInfo
std::map<std::string, double> Player::commonInfo =
    std::map<std::string, double>();

//  copy constructor
Player::Player(const Player& other)
    : name(other.name),
      score(other.score),
      actions(other.actions),
      actionPossibility(other.actionPossibility),
      strategyTables(other.strategyTables),
      strategyFunc(other.strategyFunc),
      strategy(other.strategy),
      strategies(other.strategies),
      vars(other.vars),
      qTable(other.qTable) {
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  this->gen =
      std::mt19937(seed);  //< generate random numbers with time-based seed
}

Player::Player(std::string const& name, int score,
               std::vector<Action> const& actions,
               const std::vector<std::string>& rowNames,
               const std::vector<std::string>& colNames) {
  this->name = name;
  this->score = score;
  this->actions = actions;
  this->actionPossibility = std::vector<double>(actions.size(), 0);
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  this->gen =
      std::mt19937(seed);  //< generate random numbers with time-based seed
  this->qTable = QTable(rowNames, colNames);
}

Player::~Player() {}

/**
 * @brief
 *
 * return the action of this round by reputation
 * by this->strategy, select the corresponding strategyTable,
 * strategyTable's last row is output, other rows are input, and correspond to
 * the order of the parameters
 *
 * @param recipientReputation
 * @param action_error_p The probability of action mutation
 * @return Action
 */
Action Player::donate(std::string const& recipientReputation, double epsilon, double beta_boltzmann,
                      double action_error_p, bool train, bool with_boltzmann) {
  Action resAction;
  if (train) {
    // use qTable to get action (epsilon-greedy)
    resAction =
        this->getActionFromQTable(recipientReputation, epsilon, beta_boltzmann, with_boltzmann);
  } else {
    // use strategyTable to get action
    resAction = this->getActionFromStrategyTable(this->strategy.getName(),
                                                 recipientReputation);
  }

  // there is a probability of action_error_p to mutate
  if (this->getProbability() < action_error_p) {
    std::vector<Action> alterActions;
    for (Action action : this->actions) {
      if (action.getName() != resAction.getName()) {
        alterActions.push_back(action);
      }
    }
    resAction = this->getRandomAction(alterActions);
  }
  return resAction;
}

Action Player::reward(std::string const& donorActionName, double epsilon, double beta_boltzmann,
                      double action_error_p, bool train, bool with_boltzmann) {
  Action resAction;
  if (train) {
    // use qTable to get action (epsilon-greedy)
    resAction =
        this->getActionFromQTable(donorActionName, epsilon, beta_boltzmann, with_boltzmann);
  } else {
    resAction = this->getActionFromStrategyTable(this->strategy.getName(),
                                                 donorActionName);
  }

  // there is a probability of action_error_p to mutate
  if (this->getProbability() < action_error_p) {
    std::vector<Action> alterActions;
    for (Action action : this->actions) {
      if (action.getName() != resAction.getName()) {
        alterActions.push_back(action);
      }
    }
    resAction = this->getRandomAction(alterActions);
  }

  return resAction;
}

void Player::setActionPossibility(
    const std::vector<double>& actionPossibility) {
  double sum = 0;
  // judge if the sum of actionPossibility is 1
  for (double p : actionPossibility) {
    if (p < 0) {
      std::cerr << "actionPossibility must be positive" << std::endl;
      throw "actionPossibility must be positive";
    }
    sum += p;
  }

  if (sum != 1) {
    std::cerr << "actionPossibility must sum to 1" << std::endl;
    throw "actionPossibility must sum to 1";
  }

  // set this->actionPossibility to actionPossibility
  for (int i = 0; i < actionPossibility.size(); i++) {
    this->actionPossibility[i] = actionPossibility[i];
  }
}

/**
 * @brief judge the strategy in strategies from strategies and set it to this->strategy
 *
 * @param strategy_name
 */
void Player::setStrategy(const std::string& strategy_name) {
  bool existed = false;
  for (Strategy s : this->strategies) {
    if (strategy_name == s.getName()) {
      this->strategy = s;
      existed = true;
      break;
    }
  }
  if (existed != true) {
    std::cerr << "not exist: " << strategy_name << std::endl;
    throw "not exist: " + strategy_name;
  }
}

/**
 * @brief from ${strategyPath}/${Player.name}/${strategyName}.csv load strategy
 *
 * @param strategyPath
 */
void Player::loadStrategy(const std::string& strategyPath) {
  for (auto strategy : this->strategies) {
    std::string strategyName = strategy.getName();
    std::string strategyCSVPath =
        strategyPath + "/" + this->name + "/" + strategyName + ".csv";

    try {
      // std::cout << "strategyCSVPath: " << strategyCSVPath << std::endl;
      std::ifstream csvFile(strategyCSVPath);
      if (!csvFile.is_open()) {
        std::cerr << "Failed to open file: " << strategyCSVPath << std::endl;
        throw "strategy file not found";
      }

      std::string line;
      while (std::getline(csvFile, line)) {
        // remove "\r" in line
        std::string target = "\r";
        int pos = line.find(target);
        int n = line.size();
        if (pos != std::string::npos) {
          line.erase(pos, target.size());
        }
        std::vector<std::string> strategyTableRow;
        std::stringstream ss(line);
        std::string cell;
        while (std::getline(ss, cell, ',')) {
          strategyTableRow.push_back(cell);
        }
        this->strategyTables[strategyName].push_back(strategyTableRow);
      }

      csvFile.close();

    } catch (const std::exception& e) {
      std::cerr << e.what() << '\n';
    }

    // generate strategyFunc to accelerate table lookup
    std::string key = strategyName;
    // traverse all strategyTable by column
    for (int col = 0; col < this->strategyTables[strategyName][0].size();
         col++) {
      // traverse all strategyTable by row
      for (int row = 0; row < this->strategyTables[strategyName].size();
           row++) {
        // if it is the last row, it is the output
        if (row == this->strategyTables[strategyName].size() - 1) {
          // find action from this->actions
          int finded = 0;
          for (Action action : this->actions) {
            if (action.getName() ==
                this->strategyTables[strategyName][row][col]) {
              this->strategyFunc[key] = action;
              // restore key value to initial value
              key = strategyName;
              // this->strategyFunc[key] = action.getName();
              finded = 1;
              break;
            }
          }
          if (!finded) {
            std::cerr << "action not found: "
                      << this->strategyTables[strategyName][row][col]
                      << std::endl;
            throw "action not found";
          }
        } else {
          // if it is not the last row, it is the input
          key += "!" + this->strategyTables[strategyName][row][col];
        }
      }
    }
  }
}

/** @brief use the random number generator to generate a random strategy
 *
 * @param alterStra egy
 * @return Strategy
 */
Strategy Player::getRandomOtherStrategy(std::vector<Strategy>& alterStrategy) {
  // judge if there is a selectable strategy
  if (alterStrategy.size() <= 0) {
    std::cerr << "no alter strategy" << std::endl;
    throw "no alter strategy";
  }
  std::uniform_int_distribution<int> randomDis(0, alterStrategy.size() - 1);
  int randInt = randomDis(this->gen);
  return alterStrategy[randInt];
}

double Player::getProbability() {
  std::uniform_real_distribution<double> randomDis(0, 1);
  double randDouble = randomDis(this->gen);
  return randDouble;
}

/**
 * @brief randomly output an integer in [start, end]
 *
 * @param input
 * @return int
 */
int Player::getRandomInt(int start, int end) {
  // from [start, end] get a random integer
  std::uniform_int_distribution<int> randomInt(
      start, end);  
  int randInt = randomInt(this->gen);
  return randInt;
}

Action Player::getActionFromStrategyTable(const std::string& strategyName,
                                          const std::string& input) const {
  // construct key from input (for donor, it is reputation; for recipient, it is
  // actionName)
  std::string key = strategyName;
  key += "!" + input;
  Action resAction = this->strategyFunc.at(key);
  return resAction;
}

/**
 * @brief input a character (the character may be reputation or actionName)
 *
 * @param input  reputation / actionName
 * @return Action
 */
Action Player::getActionFromQTable(const std::string& input, double epsilon,
                                   double beta_boltzmann, bool with_boltzmann) {
  Action resAction;
  if (with_boltzmann) {
    auto [actionName, actionId] =
        this->qTable.getActionByBoltzmann(input, beta_boltzmann);
    resAction.setName(actionName);
    resAction.setId(actionId);
  } else {
    if (this->getProbability() < epsilon) {
      return this->getRandomAction(this->actions);
    } else {
      auto [actionName, actionId] = this->qTable.getBestOutput(input);
      resAction.setName(actionName);
      resAction.setId(actionId);
    }
  }
  assert(resAction.getName() != "");
  return resAction;
}

Action Player::getRandomAction(std::vector<Action> const& alterActions) {
  // judge if there is a selectable strategy
  if (alterActions.size() <= 0) {
    std::cerr << "no alter action" << std::endl;
    throw "no alter action";
  }
  std::uniform_int_distribution<int> randomDis(0, alterActions.size() - 1);
  int randInt = randomDis(this->gen);
  return alterActions[randInt];
}

void Player::updateQTable(std::vector<Transition> const& transitions,
                          double alpha, double discount) {
  for (auto transition : transitions) {
    this->qTable.update(transition, alpha, discount);
  }
}

std::string Player::getStrategyNameFromQTable(int player_type) {
  std::string res;
  // donor
  if (player_type == 0) {
    std::string input_0 = "0";
    auto [action_name_0, action_id_0] = this->qTable.getBestOutput(input_0);
    std::string input_1 = "1";
    auto [action_name_1, action_id_1] = this->qTable.getBestOutput(input_1);
    if (action_name_0 == "C" and action_name_1 == "C") {
      res = "C";
    } else if (action_name_0 == "C" and action_name_1 == "D") {
      res = "NDISC";
    } else if (action_name_0 == "D" and action_name_1 == "C") {
      res = "DISC";
    } else if (action_name_0 == "D" and action_name_1 == "D") {
      res = "D";
    } else {
      std::cerr << "action name error" << std::endl;
      throw "action name error";
    }
  } else if (player_type == 1) {
    std::string input_c = "C";
    auto [action_name_c, action_id_c] = this->qTable.getBestOutput(input_c);
    std::string input_d = "D";
    auto [action_name_d, action_id_d] = this->qTable.getBestOutput(input_d);
    if (action_name_c == "C" and action_name_d == "C") {
      res = "UR";
    } else if (action_name_c == "C" and action_name_d == "D") {
      res = "SR";
    } else if (action_name_c == "D" and action_name_d == "C") {
      res = "AR";
    } else if (action_name_c == "D" and action_name_d == "D") {
      res = "NR";
    } else {
      std::cerr << "action name error" << std::endl;
      throw "action name error";
    }
  }

  return res;
}
