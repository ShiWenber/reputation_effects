/**
 * @file Buffer.hpp
 * @author ShiWenber (1210169842@qq.com)
 * @brief 经验回放池
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
  std::vector<PoolDeque<Transition>> buffer;

 public:
  Buffer(int population, int capacity);
  ~Buffer();

  void add(int index, Transition transition);

  std::vector<Transition> sample(int index, int batchSize);
};

#endif /*BUFFER_HPP*/