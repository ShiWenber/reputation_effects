#include "Norm.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <chrono>

Norm::Norm() {}

Norm::Norm(std::string csvPath) {
  this->loadNormFunc(csvPath);
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  this->gen = std::mt19937(seed);  //< generate random numbers with time-based seed
}

double Norm::getProbability() {
  std::uniform_real_distribution<double> randomDis(0, 1);
  double randDouble = randomDis(this->gen);
  return randDouble;
}

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
      // remove the line in the end "r"
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

  // generated strategyFunc, which is used to speed up the look-up table
  std::string key = "";
  // according to iterate over all strategyTable list
  for (int col = 0; col < this->normTableStr[0].size(); col++) {
    // by strategyTable traveled through all
    for (int row = 0; row < this->normTableStr.size(); row++) {
    // if the last line is for the output
      if (row == this->normTableStr.size() - 1) {
        this->normFunc[key] = std::stod(this->normTableStr[row][col]);
        // restore the key value of the initial value
        key = "";
      } else {
        // if it wasn't the last line, for the input
        key += "!" + this->normTableStr[row][col];
      }
    }
  }
}

double Norm::getReputation(Action const& donorAction,
                           Action const& recipientAction,
                           double const reputation_error_p) {
  std::string key =
      "!" + donorAction.getName() + "!" + recipientAction.getName();
  double res = this->normFunc.at(key);
  
  if (reputation_error_p == 0.0) {
    return res;
  }
  if (this->getProbability() < reputation_error_p) {
    if (res == 1.0) {
      res = 0.0;
    } else if (res == 0.0) {
      res = 1.0;
    } else {
      std::cerr << "wrong reputation value: " << res << std::endl;
      throw "wrong reputation value: " + std::to_string(res);
    }
  }

  return res;
}
