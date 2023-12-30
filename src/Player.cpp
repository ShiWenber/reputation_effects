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

// /**
//  * @brief 用轮盘法选择动作
//  *
//  * @return Action 抽取出的动作，也是本轮采取的动作
//  */
// Action Player::play() {
//   // 用时间种子生成一个0到1之间的随机数
//   std::uniform_real_distribution<double> randomDis(0, 1);
//   double randomNum = randomDis(this->gen);

//   // 轮盘法选择动作
//   double sum = 0;
//   for (int i = 0; i < this->actionPossibility.size(); i++) {
//     sum += this->actionPossibility[i];
//     if (randomNum < sum) {
//       return this->actions[i];
//     }
//   }
//   return this->actions[this->actions.size() - 1];
// }

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
  // 判断actionPossibility之和是否为1
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

  // 将this->actionPossibility设置为actionPossibility
  for (int i = 0; i < actionPossibility.size(); i++) {
    this->actionPossibility[i] = actionPossibility[i];
  }
}

/**
 * @brief 从 strategy 中查找指定 name 的 strategy
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
 * @brief 从 ${strategyPath}/${Player.name}/${strategyName}.csv 中加载策略
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

    // TODO: 将输入的表转置可以节省复杂度
    // 生成strategyFunc，用来加速查表
    std::string key = strategyName;
    // 按列遍历所有的strategyTable
    for (int col = 0; col < this->strategyTables[strategyName][0].size();
         col++) {
      // 按行遍历所有的strategyTable
      for (int row = 0; row < this->strategyTables[strategyName].size();
           row++) {
        // 如果是最后一行，则为输出
        if (row == this->strategyTables[strategyName].size() - 1) {
          // 从 this->actions 中查找该动作
          int finded = 0;
          for (Action action : this->actions) {
            if (action.getName() ==
                this->strategyTables[strategyName][row][col]) {
              this->strategyFunc[key] = action;
              // 还原 key 值为初始值
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
          // 如果不是最后一行，则为输入
          key += "!" + this->strategyTables[strategyName][row][col];
        }
      }
    }
  }
}

/** @brief  使用自带的随机数生成器生成一个随机的策略(等概率的)
 *
 * @param alterStra egy
 * @return Strategy
 */
// TODO: --
Strategy Player::getRandomOtherStrategy(std::vector<Strategy>& alterStrategy) {
  // 判断是否有可选的策略
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
 * @brief 输出随机整数 [start, end]
 *
 * @param input
 * @return int
 */
int Player::getRandomInt(int start, int end) {
  std::uniform_int_distribution<int> randomInt(
      start, end);  //< 从 [start, end]中取随机整数
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
 * @brief 输入一个字符(字符可能是 reputation 或者 actionName)
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
  // 判断是否有可选的策略
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
