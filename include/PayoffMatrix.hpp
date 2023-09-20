#ifndef PAYOFFMATRIX_HPP
#define PAYOFFMATRIX_HPP

#include <vector>
#include <set>
#include <string>
#include <map>
#include "Strategy.hpp"

class PayoffMatrix {
 private:
  std::vector<std::vector<std::vector<std::string>>> payoffMatrixStr; //< 三维的收益矩阵，二人博弈就是三维的(视为二维矩阵则每个元素都是所有参与玩家的收益列表)，三人博弈就是四维的，以此类推，这个表示的是原始的string收益矩阵，单元格可以为表达式
  std::vector<std::vector<std::vector<double>>> payoffMatrix; //< 三维的收益矩阵，二人博弈就是三维的(视为二维矩阵则每个元素都是所有参与玩家的收益列表)，三人博弈就是四维的，以此类推，这个表示的是计算后的double收益矩阵，单元格为double
  // double* payoffMatrix; //< 三维的收益矩阵，二人博弈就是三维的(视为二维矩阵则每个元素都是所有参与玩家的收益列表)，三人博弈就是四维的，以此类推，这个表示的是计算后的double收益矩阵，单元格为double
  std::vector<Strategy> strategys;
  std::map<std::string, double> vars; //< 用于存储变量名和变量值的字典
  int rowNum;
  int colNum;
  int playerNum;

 public:
  PayoffMatrix();
  PayoffMatrix(std::string csvPath);
  ~PayoffMatrix();

 // TODO: 完成
  std::vector<double> getPayoff(Strategy strategyA, Strategy strategyB);
  // std::vector<double> getPayoff()

  std::vector<std::vector<std::vector<double>>> getPayoffMatrix() const { return this->payoffMatrix; }
  void setPayoffMatrix(const std::vector<std::vector<std::vector<double>>> &payoffMatrix) { this->payoffMatrix = payoffMatrix; }

  std::vector<Strategy> getStrategys() const { return this->strategys; }
  void setStrategys(const std::vector<Strategy> &strategys) { this->strategys = strategys; }

  std::map<std::string, double> getVars() const { return this->vars; }
  void setVars(const std::map<std::string, double> &vars) { this->vars = vars; }
  void addVar(const std::string &varName, double varValue) { this->vars[varName] = varValue; }
  void removeVar(const std::string &varName) { this->vars.erase(varName); }
  void clearVars() { this->vars.clear(); }
  void updateVar(const std::string &varName, double varValue) { this->vars[varName] = varValue; }
  double getVarValue(const std::string &varName) const { return this->vars.at(varName); }

  std::vector<std::vector<std::vector<double>>> evalPayoffMatrix();

  int getRowNum() const { return this->rowNum; }

  int getColNum() const { return this->colNum; }

  int getPlayerNum() const { return this->playerNum; }

  std::vector<std::vector<std::vector<std::string>>> getPayoffMatrixStr() const { return this->payoffMatrixStr; }
};



#endif // !PAYOFFMATRIX_HPP