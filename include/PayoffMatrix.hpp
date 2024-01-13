#ifndef PAYOFFMATRIX_HPP
#define PAYOFFMATRIX_HPP

#include <vector>
#include <set>
#include <string>
#include <map>
#include "Strategy.hpp"

class PayoffMatrix {
 private:
  std::vector<std::vector<std::vector<std::string>>> payoffMatrixStr;//< payoff matrix, the 3 d game is two three-dimensional (as a two-dimensional matrix of each element was all players involved in earnings list), three people
  std::vector<std::vector<std::vector<double>>> payoffMatrix;//< payoff matrix, the 3 d game is two three-dimensional (as a two-dimensional matrix of each element was all players involved in earnings list), three people
  std::vector<Strategy> colStrategies;
  std::vector<Strategy> rowStrategies;
  std::map<std::string, double> vars; //< is used to store variable names and values of the dictionary
  int rowNum;
  int colNum;
  int playerNum;

 public:
  PayoffMatrix();
  PayoffMatrix(std::string csvPath);
  ~PayoffMatrix();

  std::vector<double> getPayoff(const Strategy& strategyA,const Strategy& strategyB) const;
  // std::vector<double> getPayoff()
  std::vector<std::vector<std::vector<double>>> getPayoffMatrix() const { return this->payoffMatrix; }
  void setPayoffMatrix(const std::vector<std::vector<std::vector<double>>> &payoffMatrix) { this->payoffMatrix = payoffMatrix; }

  std::map<std::string, double> getVars() const { return this->vars; }
  void setVars(const std::map<std::string, double> &vars) { this->vars = vars; }
  void addVar(const std::string &varName, double varValue) { this->vars[varName] = varValue; }
  void removeVar(const std::string &varName) { this->vars.erase(varName); }
  void clearVars() { this->vars.clear(); }
  void updateVar(const std::string &varName, double varValue) { this->vars[varName] = varValue; }
  double getVarValue(const std::string &varName) const { return this->vars.at(varName); }

  std::vector<std::vector<std::vector<double>>> evalPayoffMatrix();
  std::vector<std::vector<std::vector<double>>> evalPayoffMatrix( std::map<std::string, double> const & vars_for_donor, std::map<std::string, double> const & vars_for_receiver);

  int getRowNum() const { return this->rowNum; }

  int getColNum() const { return this->colNum; }

  int getPlayerNum() const { return this->playerNum; }

  std::vector<std::vector<std::vector<std::string>>> getPayoffMatrixStr() const { return this->payoffMatrixStr; }

  std::vector<Strategy> getColStrategies() const { return this->colStrategies; }
  void setColStrategies(const std::vector<Strategy> &colStrategies) { this->colStrategies = colStrategies; }

  std::vector<Strategy> getRowStrategies() const { return this->rowStrategies; }
  void setRowStrategies(const std::vector<Strategy> &rowStrategies) { this->rowStrategies = rowStrategies; }
};



#endif // !PAYOFFMATRIX_HPP