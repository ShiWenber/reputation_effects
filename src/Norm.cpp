#include "Norm.hpp"

#include <fstream>
#include <iostream>
#include <sstream>

Norm::Norm() {}

Norm::Norm(std::string csvPath) { this->loadNormFunc(csvPath); }

Norm::~Norm() {}

void Norm::loadNormFunc(std::string csvPath) {
  try {
    // std::cout << "normcsvPath: " << csvPath << std::endl;
    std::ifstream csvFile(csvPath);
    if (!csvFile.is_open()) {
      std::cerr << "Failed to open file: " << csvPath << std::endl;
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
      std::vector<std::string> normTableRow;
      std::stringstream ss(line);
      std::string cell;
      while (std::getline(ss, cell, ',')) {
        normTableRow.push_back(cell);
      }
      this->normTableStr.push_back(normTableRow);
    }

    csvFile.close();

  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
  }

  // TODO: 将输入的表转置可以节省复杂度
  // 生成strategyFunc，用来加速查表
  std::string key = "";
  // 按列遍历所有的strategyTable
  for (int col = 0; col < this->normTableStr[0].size(); col++) {
    // 按行遍历所有的strategyTable
    for (int row = 0; row < this->normTableStr.size(); row++) {
      // 如果是最后一行，则为输出
      if (row == this->normTableStr.size() - 1) {
        this->normFunc[key] = std::stod(this->normTableStr[row][col]);
        // 还原 key 值为初始值
        key = "";
      } else {
        // 如果不是最后一行，则为输入
        key += "!" + this->normTableStr[row][col];
      }
    }
  }
}

double Norm::getReputation(Action donorAction, Action recipientAction) {
  std::string key = "!" + donorAction.getName() + "!" + recipientAction.getName();
  double res = this->normFunc[key];
  if (res == 0.0) {
    // 判断this->normFunc中是否有key
    if (this->normFunc.find(key) == this->normFunc.end()) {
      // 如果没有，则异常，并抛出key
      std::cerr << "normFunc not found key: " << key << std::endl;
      throw "normFunc not found key: " + key;
    }
  }

  return res;
}
