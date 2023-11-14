#ifndef QTABLE_HPP
#define QTABLE_HPP
#include <map>
#include <vector>
#include <utility>

#include "BiMap.hpp"

class QTable {
 private:
  BiMap<std::string, int> rowIndex;
  BiMap<std::string, int> colIndex;
  std::vector<std::vector<double>> table;

 public:
  QTable(std::vector<std::string> rowNames, std::vector<std::string> colNames);
  QTable(const QTable &other);
  QTable();

  ~QTable();

  double loc(std::string rowName, std::string colName) const {
    return this->table[this->rowIndex.at(rowName)][this->colIndex.at(colName)];
  }

  double iloc(int row, int col) const { return this->table[row][col]; }

  std::vector<double> getRow(std::string rowName) const {
    return this->table[this->rowIndex.at(rowName)];
  }

  std::pair<std::string, int> getBestOutput(std::string rowName) const;
  




  std::vector<std::vector<double>> getTable() const { return this->table; }
  void setTable(const std::vector<std::vector<double>> &table) {
    this->table = table;
  }

  BiMap<std::string, int> getRowIndex() const { return this->rowIndex; }
  void setRowIndex(const BiMap<std::string, int> &rowIndex) { this->rowIndex = rowIndex; }

  BiMap<std::string, int> getColIndex() const { return this->colIndex; }
  void setColIndex(const BiMap<std::string, int> &colIndex) { this->colIndex = colIndex; }

};

#endif