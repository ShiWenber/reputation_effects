#include "QTable.hpp"

#include <Action.hpp>
#include <chrono>
#include <limits>
#include <string>
#include <vector>

/**
 * @brief Construct a new QTable::QTable object the elements in table are initialized to 0
 * 
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
  this->randomDis = std::uniform_real_distribution<double>(0, 1);

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
    : rowIndex(other.rowIndex), colIndex(other.colIndex), table(other.table), randomDis(other.randomDis) {
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  this->gen = std::mt19937(seed);
}

QTable::QTable() {}

QTable::~QTable() {}

double QTable::getProbability() {
  double randDouble = this->randomDis(this->gen);
  return randDouble;
}

/**
 * @brief input rowName, return the
 * colName and colIndex
 *
 *
 * @param rowName
 * @return std::pair<std::string, int> (colName, colIndex)
 */
std::pair<std::string, int> QTable::getBestOutput(std::string rowName) {
  std::vector<double> row_qs = this->table[this->rowIndex.at(rowName)];
  std::vector<int> max_indices;

  // int maxIndex = 0;
  double max = row_qs[0];
  max_indices.push_back(0);

  for (int i = 1; i < row_qs.size(); i++) {
    if (row_qs[i] > max) {
      max = row_qs[i];
      max_indices.clear();
      max_indices.push_back(i);
    } else if (row_qs[i] == max) {
      max_indices.push_back(i);
    }
  }
  std::uniform_int_distribution<int> randomDis(0, max_indices.size() - 1);
  int random_max_index = max_indices[randomDis(this->gen)];
  return std::pair<std::string, int>(this->colIndex.getKey(random_max_index),
                                     random_max_index);
}

/**
 * @brief get action by boltzmann distribution
 * each action get the probability of exp(q/temperature)/sum(exp(q/temperature))
 *
 * mathematically, the probability of action $j$ in time $t$:
 * $$
 * x_t(a_j)=\frac{e^{beta Q(s_t, a_j)}}{\sum_{\forall a\in\mathcal{A}}e^{beta
 * Q(s_t,a)}}
 * $$
 *
 * @param rowName
 * @param beta the parameter of boltzmann distribution, equal to 1/temperture.
 * When beta
 * -> inf, the distribution is close to argmax. When temperature -> 0, the
 * distribution is close to uniform distribution.
 *
 * @return std::pair<std::string, int> (colName, colIndex)
 */
std::pair<std::string, int> QTable::getActionByBoltzmann(
    std::string const& rowName, double beta) {
  std::vector<double> const& row_qs = this->table[this->rowIndex.at(rowName)];
  double exp_sum = 0;
  for (int i = 0; i < row_qs.size(); i++) {
    exp_sum += std::exp(beta * row_qs[i]);
  }

  std::vector<double> action_probs = std::vector<double>(row_qs.size());
  for (int i = 0; i < row_qs.size(); i++) {
    action_probs[i] = std::exp(beta * row_qs[i]) / exp_sum;
  }
  // extract action by roulette
  double p = this->getProbability();
  for (int i = 0; i < action_probs.size(); i++) {
    p -= action_probs[i];
    if (p <= 0) {
      return std::pair<std::string, int>(this->colIndex.getKey(i), i);
    }
  }

  // if no ation is selected, return the last action (this is to avoid the
  // circumstance that p > 0 because of the precision of double)
  return std::pair<std::string, int>(
      this->colIndex.getKey(action_probs.size() - 1), action_probs.size() - 1);
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