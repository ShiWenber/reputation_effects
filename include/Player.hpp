#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <map>
#include <string>
#include <unordered_map>
// 有序集合类型
#include <iostream>
#include <random>
#include <set>
#include <vector>

#include "Action.hpp"
#include "PayoffMatrix.hpp"

class Player {
  /**Using the characteristics of static variable to maintain the public
   * information, the data type is map, key for string, value for double */
 private:
  static std::map<std::string, double> commonInfo;

  std::string name;
  int score;

  std::vector<Action> actions;
  std::vector<double> actionPossibility; //< random action probability (mixed strategy)

  std::mt19937 gen; //< double random number generator

  std::map<std::string, std::vector<std::vector<std::string>>>
      strategyTables; //< action function table of each strategy
  std::unordered_map<std::string, Action>
      strategyFunc;  //<
                     // Action function of each policy. key consists of the key of the strategyTables and all inputs. value is the action name
  Strategy strategy; //< current strategy

  std::vector<Strategy> strategies;

  double deltaScore;  //< The change in revenue from the last upScore

  std::map<std::string, double> vars;

 public:
  Player(const Player &other);
  Player(std::string name, int score, std::vector<Action> actions);
  ~Player();

  /**According to the requirements of the input to return to an action, depending on the strategyTables*/
  Action donate(std::string const &recipientReputation,
                double action_error_p = 0);

  Action reward(std::string const &donorActionName, double action_error_p = 0);

  /** according to the query to the income of the delta value, update the
   * score*/
  void updateScore(double delta) {
    this->deltaScore += delta;
    this->score += delta;
  };

  void clearDeltaScore() { this->deltaScore = 0; }

  std::string getName() const { return this->name; }
  void setName(const std::string &name) { this->name = name; }

  int getScore() const { return this->score; }
  void setScore(int score) { this->score = score; }

  std::vector<Action> getActions() const { return this->actions; }

  std::vector<double> getActionPossibility() const {
    return this->actionPossibility;
  }
  Strategy getStrategy() const { return this->strategy; }
  void setStrategy(const Strategy &strategy) { this->strategy = strategy; }
  void setStrategy(const std::string &strategyName);

  static std::map<std::string, double> getCommonInfo() {
    return Player::commonInfo;
  }
  static void setCommonInfo(const std::map<std::string, double> &commonInfo) {
    Player::commonInfo = commonInfo;
  }
  static void addCommonInfo(const std::string &key, const double &value) {
    Player::commonInfo[key] = value;
  }
  static void removeCommonInfo(const std::string &key) {
    Player::commonInfo.erase(key);
  }
  static void clearCommonInfo() { Player::commonInfo.clear(); }
  static void updateCommonInfo(const std::string &key, const double &value) {
    Player::commonInfo[key] = value;
  }
  static double getCommonInfoValue(const std::string &key) {
    return Player::commonInfo[key];
  }

  std::map<std::string, double> getVars() const { return this->vars; }
  void setVars(const std::map<std::string, double> &vars) { this->vars = vars; }
  void addVar(const std::string &varName, double varValue) {
    if (existVar(varName)) {
      std::cerr << "exist var: " << varName << std::endl;
      throw "exist var: " + varName;
    }
    this->vars[varName] = varValue;
  }
  void removeVar(const std::string &varName) { this->vars.erase(varName); }
  void clearVars() { this->vars.clear(); }
  void updateVar(const std::string &varName, double varValue) {
    if (!existVar(varName)) {
      std::cerr << "not exist var: " << varName << std::endl;
      throw "not exist var: " + varName;
    }
    this->vars[varName] = varValue;
  }
  void updateVar(const std::string &varName, const std::string &varValue) {
    if (!existVar(varName)) {
      std::cerr << "not exist var: " << varName << std::endl;
      throw "not exist var: " + varName;
    }
    this->vars[varName] = std::stod(varValue);
  }

  double getVarValue(const std::string &varName) const {
    return this->vars.at(varName);
  }

  bool existVar(const std::string &varName) const {
    int existed = 0;
    for (auto var : this->vars) {
      if (var.first == varName) {
        existed = 1;
        break;
      }
    }
    if (existed == 0) {
      return false;
    } else {
      return true;
    }
  }

  void loadStrategy(const std::string &strategyPath);
  std::map<std::string, std::vector<std::vector<std::string>>>
  getStrategyTables() const {
    return this->strategyTables;
  }
  std::vector<Strategy> getStrategies() const { return this->strategies; }
  void setStrategies(const std::vector<Strategy> &strategies) {
    this->strategies = strategies;
  }

  // random behavior, based on the random number generator inside the object, with the use of
  // random number generator, and random behavior cannot use cons
  Strategy getRandomOtherStrategy(std::vector<Strategy> &alterStrategy);
  Action getRandomAction(std::vector<Action> &alterAction);

  // to throw out a probability of 0 and 1
  double getProbability();
  int getRandomInt(int start, int end);

  double getDeltaScore() const { return this->deltaScore; }
};

#endif  // !PLAYER_HPP
