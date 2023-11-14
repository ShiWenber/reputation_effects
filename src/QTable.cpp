#include "QTable.hpp"


#include <string>
#include <vector>
#include <limits>


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

QTable::QTable(const QTable &other)
    : rowIndex(other.rowIndex), colIndex(other.colIndex), table(other.table) {}

QTable::QTable() {}

QTable::~QTable() {}

/**
 * @brief 输入行名，返回该行最大值的列名和列索引
 *
 * @param rowName
 * @return std::pair<std::string, int> (列名，列索引)
 */
std::pair<std::string, int> QTable::getBestOutput(std::string rowName) const {
  std::vector<double> row = this->table[this->rowIndex.at(rowName)];
  int maxIndex = 0;
  double max = row[0];
  std::string maxColName = this->colIndex.getKey(maxIndex);
  for (int i = 1; i < row.size(); i++) {
    if (row[i] > max) {
      max = row[i];
      maxIndex = i;
    }
  }
  return std::pair<std::string, int>(this->colIndex.getKey(maxIndex), maxIndex);
}
