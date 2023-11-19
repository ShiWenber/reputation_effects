#include "Player.hpp"

#include <chrono>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>

// 为静态成员初始化一个空的map
std::map<std::string, double> Player::commonInfo =
    std::map<std::string, double>();

//  拷贝构造
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
  this->gen = std::mt19937(seed);  //< 以时间为种子生成随机数
}

Player::Player(std::string name, int score, std::vector<Action> actions) {
  this->name = name;
  this->score = score;
  this->actions = actions;
  this->actionPossibility = std::vector<double>(actions.size(), 0);
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  this->gen = std::mt19937(seed);  //< 以时间为种子生成随机数
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
 * 通过输入的声誉返回该轮采取的动作，通过this->strategy选择对应的strategyTable,
 * strategyTable的最后一行为输出，其他行均为输入，并与参数的顺序对应
 *
 * TODO:
 * 可以通过设置变长参数表来实现输入的数量不固定，当前采用的是固定的两个输入:
 * this->strategy, recipientReputation
 * @param recipientReputation
 * @param mu 动作突变率
 * @return Action
 */
Action Player::donate(std::string recipientReputation, double mu) {
  // 从形参构建key
  std::string key = this->strategy.getName();
  key += "!" + recipientReputation;

  // 判断是否索引到了输出action，如果key没有对应的value，则抛出异常
  Action resAction = this->strategyFunc[key];
  if (resAction.getName() == "") {
    std::cerr << "key: " << key << " not found" << std::endl;
    throw "action not found";
  }

  // mu概率Action突变
  if (this->getProbability() < mu) {
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

Action Player::reward(std::string donorActionName, double mu) {
  // 从形参构建key
  std::string key = this->strategy.getName();
  key += "!" + donorActionName;

  // 判断是否索引到了输出action，如果key没有对应的value，则抛出异常
  Action resAction = this->strategyFunc[key];
  if (resAction.getName() == "") {
    std::cerr << "key: " << key << " not found" << std::endl;
    throw "action not found";
  }

  // mu概率Action突变
  if (this->getProbability() < mu) {
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
 * @brief 输出随机整数
 * 
 * @param input 
 * @return int 
 */
int Player::getRandomInt(int start, int end) {
  std::uniform_int_distribution<int> randomInt(start, end); //< 从 [start, end]中取随机整数
  int randInt = randomInt(this->gen);
  return randInt;
}

Action Player::getRandomAction(std::vector<Action>& alterActions) {
  // 判断是否有可选的策略
  if (alterActions.size() <= 0) {
    std::cerr << "no alter action" << std::endl;
    throw "no alter action";
  }
  std::uniform_int_distribution<int> randomDis(0, alterActions.size() - 1);
  int randInt = randomDis(this->gen);
  return alterActions[randInt];
}
