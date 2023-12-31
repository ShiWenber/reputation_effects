#include "PayoffMatrix.hpp"

#include <assert.h>
#include <muParser.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "Strategy.hpp"

PayoffMatrix::PayoffMatrix() {}

/**
 * @brief Construct a new Payoff Matrix:: Payoff Matrix object
 * TODO:
 * 对csv文件中的表达式进行字母判断，保证所有表达式中出现的变量名都被第一行第一列的单元格声明过
 * TODO: 这里读出了博弈者名称，可以直接在该函数中初始化Player
 *
 * @param csvPath
 */
PayoffMatrix::PayoffMatrix(std::string csvPath) {
  char delimiter = ',';  //< 分隔符

  int rowNum = 0;
  int colNum = 0;
  int playerNum = 0;

  // 读取csv文件
  std::ifstream csvFile(csvPath);

  int isRowOne = 1;
  std::string line;
  // 逐行读取
  while (std::getline(csvFile, line)) {
    // 从line中去除'\r'
    std::string target = "\r";
    int pos = line.find(target);
    int n = line.size();
    if (pos != std::string::npos) {
      line.erase(pos, target.size());
    }

    std::vector<std::vector<std::string>> payoffMatrixRow;
    std::stringstream ss(line);
    std::string cell;
    // 逐个单元格读取
    if (isRowOne == 1) {
      isRowOne = 0;
      // 在第一行读取动作集
      int strategyId = 0;
      int isColOne = 1;
      // 逐个单元格读取
      while (std::getline(ss, cell, delimiter)) {
        if (isColOne == 1) {
          isColOne = 0;
          // 将读取到的字符按照:分割
          std::stringstream cell_ss(cell);
          std::string valName;
          std::string players;
          // 先读掉博弈者名称统计出博弈者数量
          getline(cell_ss, players, ':');
          std::stringstream players_ss(players);
          std::string playerName;
          while (getline(players_ss, playerName, ' ')) {
            // TODO: 这里可以读出博弈者名称
            // std::cout << playerName << std::endl;
            playerNum++;
          }
          // 读表达式变量，初始值为0
          while (getline(cell_ss, valName, ' ')) {
            this->vars[valName] = 0;
          }
        } else {
          this->colStrategies.push_back(Strategy(cell, strategyId++));
        }
      }
      colNum = this->colStrategies.size();
    } else {
      rowNum++;
      int temp_colNum = 0;
      std::getline(ss, cell, delimiter);
      // the cell is the rowStrategy name
      this->rowStrategies.push_back(Strategy(cell, rowNum - 1));
      while (std::getline(ss, cell, delimiter)) {
        temp_colNum++;
        std::stringstream cell_ss(cell);
        std::string payoffForOnePlayer_str;
        std::vector<std::string> payoffList;
        int temp_playerNum = 0;
        while (std::getline(cell_ss, payoffForOnePlayer_str,
                            ':')) {  //< :分割所有参与者的收益
          temp_playerNum++;
          payoffList.push_back(payoffForOnePlayer_str);
        }
        if (temp_playerNum != playerNum) {
          std::cerr << "not every cell has the same number of players's payoff"
                    << std::endl;
          throw "not every cell has the same number of players's payoff";
        }
        payoffMatrixRow.push_back(payoffList);
      }
      if (temp_colNum != colNum) {
        // 并不是每行都有相同的元素数量
        std::cerr << "not every row has the same number of elements"
                  << std::endl;
        throw "not every row has the same number of elements";
      }
      this->payoffMatrixStr.push_back(payoffMatrixRow);
    }
  }

  this->colNum = colNum;
  this->rowNum = rowNum;
  this->playerNum = playerNum;

  //   释放所有资源
  csvFile.close();
}

PayoffMatrix::~PayoffMatrix() {}

/**
 * @brief 根据两个动作返回收益列表
 *
 * @param strategyA
 * @param strategyB
 * @return std::vector<double>
 * 返回的第一个元素是动作A的实施者的收益，第二个元素是动作B的实施者的收益，这将会把原本payoffmatrix元素中的表达式的值计算出来
 * TODO: 完成输出值从string表达式到double的转换
 */
std::vector<double> PayoffMatrix::getPayoff(const Strategy &strategyA,
                                            const Strategy &strategyB) const {
  // strategyA必须来自行策略集，strategyB必须来自列策略集
  assert(std::find(this->rowStrategies.begin(), this->rowStrategies.end(),
                   strategyA) != this->rowStrategies.end());
  assert(std::find(this->colStrategies.begin(), this->colStrategies.end(),
                   strategyB) != this->colStrategies.end());
  int idA = strategyA.getId();
  int idB = strategyB.getId();
  if (idA < this->rowStrategies.size() && idB < this->colStrategies.size()) {
    return this->payoffMatrix[idA][idB];
  } else {
    std::cerr << "strategy id not found" << std::endl;
    throw "strategy id not found";
  }
}

/**
 * @brief
 * eval the expression in payoffMatrixStr and assign the value to payoffMatrix
 *
 * @return std::vector<std::vector<std::vector<double>>>
 */
std::vector<std::vector<std::vector<double>>> PayoffMatrix::evalPayoffMatrix() {
  this->payoffMatrix = std::vector<std::vector<std::vector<double>>>(
      this->rowNum, std::vector<std::vector<double>>(
                        this->colNum, std::vector<double>(this->playerNum)));
  try {
    mu::Parser p;
    for (auto it = this->vars.begin(); it != this->vars.end(); it++) {
      p.DefineConst(it->first, it->second);
    }
    // 按照this->vars来设置变量

    for (int row = 0; row < this->payoffMatrixStr.size(); row++) {
      for (int col = 0; col < this->payoffMatrixStr[row].size(); col++) {
        for (int player = 0; player < this->payoffMatrixStr[row][col].size();
             player++) {
          // payoff表达式
          std::string payoffStrExp = this->payoffMatrixStr[row][col][player];
          p.SetExpr(payoffStrExp);
          this->payoffMatrix[row][col][player] = p.Eval();
        }
      }
    }
  } catch (mu::Parser::exception_type &e) {
    std::cout << e.GetMsg() << std::endl;
  }
  return this->payoffMatrix;
}

/**
 * @brief
 * eval the expression in payoffMatrixStr and assign the value to payoffMatrix
 * for the special var you want to assign, you can input the var map as the
 * parameter
 *
 * @return std::vector<std::vector<std::vector<double>>>
 */
std::vector<std::vector<std::vector<double>>> PayoffMatrix::evalPayoffMatrix(
    std::map<std::string, double> const & vars_for_donor,
    std::map<std::string, double> const & vars_for_receiver) {
  this->payoffMatrix = std::vector<std::vector<std::vector<double>>>(
      this->rowNum, std::vector<std::vector<double>>(
                        this->colNum, std::vector<double>(this->playerNum)));
  try {
    mu::Parser p_donor;
    for (auto it = this->vars.begin(); it != this->vars.end(); it++) {
      // if var in vars_for_donor, use the value in vars_for_donor
      if (vars_for_donor.find(it->first) != vars_for_donor.end()) {
        p_donor.DefineConst(it->first, vars_for_donor.at(it->first));
      } else {
        p_donor.DefineConst(it->first, it->second);
      }
    }
    int player_type = 0;
    for (int row = 0; row < this->payoffMatrixStr.size(); row++) {
      for (int col = 0; col < this->payoffMatrixStr[row].size(); col++) {
        std::string payoffStrExp = this->payoffMatrixStr[row][col][player_type];
        p_donor.SetExpr(payoffStrExp);
        this->payoffMatrix[row][col][player_type] = p_donor.Eval();
      }
    }
  } catch (mu::Parser::exception_type &e) {
    std::cout << e.GetMsg() << std::endl;
  }

  try {
    mu::Parser p_recipient;
    for (auto it = this->vars.begin(); it != this->vars.end(); it++) {
      if (vars_for_receiver.find(it->first) != vars_for_receiver.end()) {
        p_recipient.DefineConst(it->first, vars_for_receiver.at(it->first));
      } else {
        p_recipient.DefineConst(it->first, it->second);
      }
    }
    int player_type = 1;
    for (int row = 0; row < this->payoffMatrixStr.size(); row++) {
      for (int col = 0; col < this->payoffMatrixStr[row].size(); col++) {
        std::string payoffStrExp = this->payoffMatrixStr[row][col][player_type];
        p_recipient.SetExpr(payoffStrExp);
        this->payoffMatrix[row][col][player_type] = p_recipient.Eval();
      }
    }
  } catch (mu::Parser::exception_type &e) {
    std::cout << e.GetMsg() << std::endl;
  }

  return this->payoffMatrix;
}
