#include "RewardMatrix.hpp"

#include <muParser.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

/**
 * @brief Construct a new Reward Matrix:: Reward Matrix object
 * 从RewardMatrix.csv文件中读取奖励矩阵，并构建奖励矩阵对象
 *
 * @param csvPath
 */
RewardMatrix::RewardMatrix(std::string csvPath) {
  char delimiter = ',';

  int rowNum = 0;
  int colNum = 0;
  int playerNum = 0;

  int player1_actionId = 0;
  int player2_actionId = 0;

  std::vector<std::vector<std::vector<std::string>>> rewardMatrixStr;

  // 读取csv文件
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
    std::vector<std::vector<std::string>> rewardMatrixRow;
    std::stringstream ss(line);
    std::string cell;
    if (isRowOne == 1) {
      isRowOne = 0;
      // 在第一行读取动作集
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
          std::getline(cell_ss, players, ':');
          std::stringstream players_ss(players);
          std::string playerName;
          while (std::getline(players_ss, playerName, ' ')) {
            std::cout << playerName << std::endl;
            playerNum++;
          }
          // 读表达式变量，初始值为0
          while (getline(cell_ss, valName, ' ')) {
            this->vars[valName] = 0;
          }
        } else {
          this->player2_actions.push_back(Action(cell, player2_actionId++));
        }
      }
      colNum = this->player2_actions.size();
    } else {
      rowNum++;
      int temp_colNum = 0;
      std::getline(ss, cell, delimiter);  //< 读出第一列的动作名
      this->player1_actions.push_back(Action(cell, player2_actionId++));
      while (std::getline(ss, cell, delimiter)) {
        temp_colNum++;
        std::stringstream cell_ss(cell);
        std::string rewardForOnePlayer_str;
        std::vector<std::string> rewardList;
        int temp_playerNum = 0;
        while (std::getline(cell_ss, rewardForOnePlayer_str, ' ')) {
          temp_playerNum++;
          rewardList.push_back(rewardForOnePlayer_str);
        }
        if (temp_playerNum != playerNum) {
          std::cerr << "not every cell has the same number of players's payoff"
                    << std::endl;
          throw "not every cell has the same number of players's payoff";
        }
        rewardMatrixRow.push_back(rewardList);
      }
      if (temp_colNum != colNum) {
        // 并不是每行都有相同的元素数量
        std::cerr << "not every row has the same number of elements"
                  << std::endl;
        throw "not every row has the same number of elements";
      }
      this->rewardMatrixStr.push_back(rewardMatrixRow);
    }
  }

  this->rowNum = rowNum;
  this->colNum = colNum;
  this->playerNum = playerNum;

  csvFile.close();
}

/**
 * @brief 将rewardMatrix中的表达式计算出值，赋值给rewardMatrix
 *
 * @return std::vector<std::vector<std::vector<double>>>
 */
std::vector<std::vector<std::vector<double>>> RewardMatrix::evalRewardMatrix() {
  this->rewardMatrix = std::vector<std::vector<std::vector<double>>>(
      this->rowNum, std::vector<std::vector<double>>(
                        this->colNum, std::vector<double>(this->playerNum, 0)));
  try {
    mu::Parser p;
    for (auto it = this->vars.begin(); it != this->vars.end(); it++) {
      p.DefineConst(
          it->first,
          it->second);  //< 定义变量 first 存储变量名，second 存储变量值
    }
    // 按照 this->vars 来设置变量
    for (int row = 0; row < this->rewardMatrixStr.size(); row++) {
      for (int col = 0; col < this->rewardMatrixStr[row].size(); col++) {
        for (int player = 0; player < this->rewardMatrixStr[row][col].size();
             player++) {
          std::string rewardStr = this->rewardMatrixStr[row][col][player];
          p.SetExpr(rewardStr);
          this->rewardMatrix[row][col][player] = p.Eval();
        }
      }
    }
  } catch (mu::Parser::exception_type& e) {
    std::cout << e.GetMsg() << std::endl;
  }
  return this->rewardMatrix;
}

void RewardMatrix::print() const {
  std::cout << "---RewardMatrix---<<" << std::endl;
  for (int r = 0; r < this->getRowNum(); r++) {
    for (int c = 0; c < this->getColNum(); c++) {
      for (int p = 0; p < this->getPlayerNum(); p++) {
        std::cout << this->getRewardMatrix()[r][c][p] << ",";
      }
      std::cout << "\t";
    }
    std::cout << std::endl;
  }
  std::cout << "-----------------<<" << std::endl;
}

std::vector<double> RewardMatrix::getReward(Action player1_action,
                                            Action player2_action) {
  int id1 = player1_action.getId();
  int id2 = player2_action.getId();
  if (id1 < this->player1_actions.size() && id2 < this->player2_actions.size()) {
    return this->rewardMatrix[id1][id2];
  } else {
    std::cerr << "action id not found" << std::endl;
    throw "action id not found";
  }
}

RewardMatrix::~RewardMatrix() {}