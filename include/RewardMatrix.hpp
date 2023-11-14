#ifndef REWARDMATRIX_HPP
#define REWARDMATRIX_HPP

#include <map>
#include <vector>

#include "Action.hpp"

class RewardMatrix {
 private:
  /** 存储表达式 */
  std::vector<std::vector<std::vector<std::string>>> rewardMatrixStr;
  /** 存储数值 */
  std::vector<std::vector<std::vector<double>>> rewardMatrix;
  int rowNum;
  int colNum;
  int playerNum;

  std::vector<Action> player1_actions;
  std::vector<Action> player2_actions;
  std::map<std::string, double> vars;  //< 用于存储变量名和变量值的字典

 public:
  RewardMatrix(std::string csvFile);
  ~RewardMatrix();

  std::vector<std::vector<std::vector<double>>> evalRewardMatrix();
  std::map<std::string, double> getVars() const { return this->vars; }
  void updateVar(const std::string &varName, double varValue) { this->vars[varName] = varValue; }
  std::vector<std::vector<std::vector<double>>> getRewardMatrix() const { return this->rewardMatrix; }
  int getPlayerNum() const { return this->playerNum; }
  int getColNum() const { return this->colNum; }
  int getRowNum() const { return this->rowNum; }
  void print() const;
  std::vector<double> getReward(Action player1_action, Action player2_action);

  

};

#endif /*REWARDMATRIX_HPP*/

  