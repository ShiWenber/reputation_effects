#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <map>
#include <string>
#include <unordered_map>
#include <iostream>
#include <random>
#include <set>
#include <vector>

#include "Action.hpp"
#include "QTable.hpp"
#include "Strategy.hpp"
#include "Transition.hpp"

class Player {
 private:
  /** make use of the static variable to maintain the common information, the
   * data type is map, the key is string, and the value is double */
  static std::map<std::string, double> commonInfo;
  std::string name;
  int score;

  std::vector<Action> actions;

  //! random action possibility(mixed strategy)
  std::vector<double> actionPossibility;

  //! random double generator
  std::mt19937 gen;

  //! the action function table of each strategy
  std::map<std::string, std::vector<std::vector<std::string>>> strategyTables;

  //!  the action function of each strategy, key is the strategy name, value is
  //!  the action
  std::unordered_map<std::string, Action> strategyFunc;
  //! current strategy
  Strategy strategy;

  std::vector<Strategy> strategies;

  std::map<std::string, double> vars;

  /** diff with strategy, q-table is a 2-d array which need to be trained
   * with need to train, and strategy is a function for the determined strategy */
  QTable qTable;

 public:
  Player(const Player &other);
  Player(std::string const &name, int score, std::vector<Action> const &actions,
         const std::vector<std::string> &rowNames,
         const std::vector<std::string> &colNames);
  ~Player();

  /** according the input, return an action, need strategyTables */
  Action donate(std::string const &recipientReputation, double epsilon,
                double beta_boltzmann, double action_error_p = 0,
                bool train = false, bool with_boltzmann = false);

  Action reward(std::string const &donorActionName, double epsilon,
                double beta_boltzmann, double action_error_p = 0,
                bool train = false, bool with_boltzmann = false);

  std::string getName() const { return this->name; }
  void setName(const std::string &name) { this->name = name; }

  int getScore() const { return this->score; }
  void setScore(int score) { this->score = score; }

  std::vector<Action> getActions() const { return this->actions; }

  std::vector<double> getActionPossibility() const {
    return this->actionPossibility;
  }
  void setActionPossibility(const std::vector<double> &actionPossibility);

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

  /** random action based on the random generator in the object. for the
   * convenience of the test, the random action can not be const */
  Strategy getRandomOtherStrategy(std::vector<Strategy> &alterStrategy);
  Action getRandomAction(std::vector<Action> const &alterAction);

  //! return a probability 0-1
  double getProbability();
  int getRandomInt(int start, int end);

  // return the action according to the strategy table
  Action getActionFromStrategyTable(
      const std::string &strategyName,
      const std::string &recipientReputation) const;

  // return the action according to the q table
  Action getActionFromQTable(const std::string &input, double epsilon,
                             double beta_boltzmann,
                             bool with_boltzmann = false);

  void updateQTable(std::vector<Transition> const &transitions, double alpha,
                    double discount);

  //! Get the Strategy Name From Q Table object
  std::string getStrategyNameFromQTable(int player_type);
};

#endif  // !PLAYER_HPP
