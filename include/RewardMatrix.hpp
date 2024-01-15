#ifndef REWARDMATRIX_HPP
#define REWARDMATRIX_HPP

#include <map>
#include <vector>

#include "Action.hpp"

class RewardMatrix {
 private:
  /** restore the expression */
  std::vector<std::vector<std::vector<std::string>>> rewardMatrixStr;
  /** restore the value */
  std::vector<std::vector<std::vector<double>>> rewardMatrix;
  int rowNum;
  int colNum;
  int playerNum;

  std::vector<Action> player1_actions;
  std::vector<Action> player2_actions;
  std::map<std::string, double> vars;  //< the dictionary to store the variable name and value

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
  std::vector<double> getReward(Action const& player1_action, Action const& player2_action) const;

};

#endif /*REWARDMATRIX_HPP*/

  