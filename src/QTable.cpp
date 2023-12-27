#include "QTable.hpp"

#include <Action.hpp>
#include <chrono>
#include <limits>
#include <string>
#include <vector>

/**
 * @brief Construct a new QTable::QTable object table中的元素初始化为0
 *
 * @param rowNames
 * @param colNames
 */
QTable::QTable(std::vector<std::string> rowNames,
               std::vector<std::string> colNames) {
  this->rowIndex = BiMap<std::string, int>();
  this->colIndex = BiMap<std::string, int>();
  this->table = std::vector<std::vector<double>>();
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  this->gen = std::mt19937(seed);

  for (int i = 0; i < rowNames.size(); i++) {
    this->rowIndex.insert(rowNames[i], i);
  }

  for (int i = 0; i < colNames.size(); i++) {
    this->colIndex.insert(colNames[i], i);
  }

  for (int i = 0; i < rowNames.size(); i++) {
    std::vector<double> row;
    for (int j = 0; j < colNames.size(); j++) {
      row.push_back(0);
    }
    this->table.push_back(row);
  }
}

QTable::QTable(const QTable& other)
    : rowIndex(other.rowIndex), colIndex(other.colIndex), table(other.table) {
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  this->gen = std::mt19937(seed);
}

QTable::QTable() {}

QTable::~QTable() {}

/**
 * @brief 输入行名，返回该行最大值的列名和列索引 input rowName, return the colName and colIndex 
 * 
 *
 * @param rowName
 * @return std::pair<std::string, int> (列名，列索引)
 */
std::pair<std::string, int> QTable::getBestOutput(std::string rowName) {
  std::vector<double> row = this->table[this->rowIndex.at(rowName)];
  std::vector<int> max_indices;

  // int maxIndex = 0;
  double max = row[0];
  max_indices.push_back(0);

  for (int i = 1; i < row.size(); i++) {
    if (row[i] > max) {
      max = row[i];
      max_indices.clear();
      max_indices.push_back(i);
    } else if (row[i] == max) {
      max_indices.push_back(i);
    }
  }
  std::uniform_int_distribution<int> randomDis(0, max_indices.size() - 1);
  int random_max_index = max_indices[randomDis(this->gen)];
  return std::pair<std::string, int>(this->colIndex.getKey(random_max_index),
                                     random_max_index);
}

void QTable::update(Transition const& transitions, double alpha,
                    double discount) {
  std::string const& state = transitions.getState();
  std::string const& next_state = transitions.getNextState();
  Action const& action = transitions.getAction();
  double reward = transitions.getReward();

  double q = this->loc(state, action.getName());
  double max_q = this->loc(next_state, this->getBestOutput(next_state).first);
  double new_q = q + alpha * (reward + discount * max_q - q);
  this->table[this->rowIndex.at(state)][this->colIndex.at(action.getName())] =
      new_q;
  // TODO: record update times
  // update_count++
}