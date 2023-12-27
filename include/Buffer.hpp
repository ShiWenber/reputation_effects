/**
 * @file Buffer.hpp
 * @author ShiWenber (1210169842@qq.com)
 * @brief experience replay buffer pool
 * @version 0.1
 * @date 2023-11-02
 *
 * @copyright Copyright (c) 2023
 *
 */


#ifndef BUFFER_HPP
#define BUFFER_HPP



#include <vector>

#include "PoolDeque.hpp"
#include "Transition.hpp"

class Buffer {
 private:
 //! _pool_deques[t][i] as the experience pool of player t whose action as player t
  std::vector<std::vector<PoolDeque<Transition>>> _pool_deques;
  int _player_type_num;

 public:
  Buffer(int player_type_num, int population, int capacity);
  ~Buffer();

  void add(int player_type, int index, Transition const& transition);
  int size(int player_type, int index) const { return this->_pool_deques[player_type][index].size(); }

  std::vector<Transition> sample(int player_type, int index, int batchSize);
};

#endif /*BUFFER_HPP*/