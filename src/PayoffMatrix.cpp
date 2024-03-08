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
 *
 * @param csvPath
 */
PayoffMatrix::PayoffMatrix(std::string csvPath) {
  char delimiter = ',';

  int rowNum = 0;
  int colNum = 0;
  int playerNum = 0;

  std::ifstream csvFile(csvPath);

  int isRowOne = 1;
  std::string line;
  while (std::getline(csvFile, line)) {
    std::string target = "\r";
    int pos = line.find(target);
    int n = line.size();
    if (pos != std::string::npos) {
      line.erase(pos, target.size());
    }

    std::vector<std::vector<std::string>> payoffMatrixRow;
    std::stringstream ss(line);
    std::string cell;
    if (isRowOne == 1) {
      isRowOne = 0;
      int strategyId = 0;
      int isColOne = 1;
      while (std::getline(ss, cell, delimiter)) {
        if (isColOne == 1) {
          isColOne = 0;
          std::stringstream cell_ss(cell);
          std::string valName;
          std::string players;
          getline(cell_ss, players, ':');
          std::stringstream players_ss(players);
          std::string playerName;
          while (getline(players_ss, playerName, ' ')) {
            // std::cout << playerName << std::endl;
            playerNum++;
          }
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
                            ':')) {
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
        // is not have the same number of elements in a row
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

  // release the file
  csvFile.close();
}

PayoffMatrix::~PayoffMatrix() {}

/**
 * @brief return the payoff list of the two actions
 *
 * @param strategyA
 * @param strategyB
 * @return std::vector<double>
 * the first element is the payoff of the player who implement the action A, the second element is the payoff of the player who implement the action B, this will calculate the value of the expression in the payoffmatrix element
 */
std::vector<double> PayoffMatrix::getPayoff(const Strategy &strategyA,
                                            const Strategy &strategyB) const {
  // strategyA must come from the row strategy set, and strategyB must come from
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
    // according to this->vars to set the vars

    for (int row = 0; row < this->payoffMatrixStr.size(); row++) {
      for (int col = 0; col < this->payoffMatrixStr[row].size(); col++) {
        for (int player = 0; player < this->payoffMatrixStr[row][col].size();
             player++) {
          // the payoff expression
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
