#include "Player.hpp"

#include <chrono>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>

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
      vars(other.vars) {
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  this->gen = std::mt19937(seed);  //< generate random numbers with time-based seed
}

Player::Player(std::string name, int score, std::vector<Action> actions) {
  this->name = name;
  this->score = score;
  this->actions = actions;
  this->actionPossibility = std::vector<double>(actions.size(), 0);
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  this->gen = std::mt19937(seed);  //< generate random numbers with time-based seed
}

Player::~Player() {}

/**
 * @brief
 * 
 * return the action of this round by reputation
 * by this->strategy, select the corresponding strategyTable,
 * strategyTable's last row is output, other rows are input, and correspond to the order of the parameters
 *
 * @param recipientReputation
 * @param action_error_p The probability of action mutation
 * @return Action
 */
Action Player::donate(std::string const& recipientReputation, double action_error_p) {
  // construct key from parameters
  std::string key = this->strategy.getName();
  key += "!" + recipientReputation;

  Action resAction = this->strategyFunc.at(key);
  if (action_error_p == 0.0) {
    return resAction;
  }

  // Action will mutate with probability action_error_p
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

Action Player::reward(std::string const& donorActionName, double action_error_p) {
  // using the parameters to construct key
  std::string key = this->strategy.getName();
  key += "!" + donorActionName;

  // judge whether the output action is indexed, if the key does not have a corresponding value, an exception is thrown
  Action resAction = this->strategyFunc.at(key);

  if (action_error_p == 0.0) {
    return resAction;
  }

  // Action will mutate with probability action_error_p
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

/**
 * @brief find the strategy by name in the strategy set
 *
 * @param strategyName
 */
void Player::setStrategy(const std::string& strategyName) {
  bool existed = false;
  for (Strategy s : this->strategies) {
    if (strategyName == s.getName()) {
      this->strategy = s;
      existed = true;
      break;
    }
  }
  if (existed != true) {
    std::cerr << "not exist: " << strategyName << std::endl;
    throw "not exist: " + strategyName;
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
        // 清除line中的"\r"
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

    // generate strategyFunc, which is used to speed up the look-up table
    std::string key = strategyName;
    // traverse all strategyTable by column
    for (int col = 0; col < this->strategyTables[strategyName][0].size();
         col++) {
      // traverse all strategyTable by row
      for (int row = 0; row < this->strategyTables[strategyName].size();
           row++) {
        // if it is the last line, it is output
        if (row == this->strategyTables[strategyName].size() - 1) {
          // find the action from this->actions
          int finded = 0;
          for (Action action : this->actions) {
            if (action.getName() ==
                this->strategyTables[strategyName][row][col]) {
              this->strategyFunc[key] = action;
              // restore the key value of the initial value
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
          // if it is not the last line, it is input
          key += "!" + this->strategyTables[strategyName][row][col];
        }
      }
    }
  }
}

/** @brief using the built-in random number generator to generate a random strategy (equal probability)
 *
 * @param alterStrategy
 * @return Strategy
 */
Strategy Player::getRandomOtherStrategy(std::vector<Strategy>& alterStrategy) {
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
 * @brief output random integer
 * 
 * @param input 
 * @return int 
 */
int Player::getRandomInt(int start, int end) {
  std::uniform_int_distribution<int> randomInt(start, end); //< take random integers from [start, end]
  int randInt = randomInt(this->gen);
  return randInt;
}

Action Player::getRandomAction(std::vector<Action>& alterActions) {
  if (alterActions.size() <= 0) {
    std::cerr << "no alter action" << std::endl;
    throw "no alter action";
  }
  std::uniform_int_distribution<int> randomDis(0, alterActions.size() - 1);
  int randInt = randomDis(this->gen);
  return alterActions[randInt];
}
