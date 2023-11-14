/**
 * @file PoolVector.hpp
 * @author ShiWenber (1210169842@qq.com)
 * @brief 尾进头出，维持定长的可下标访存的双向队列
 * @version 0.1
 * @date 2023-11-02
 *
 * @copyright Copyright (c) 2023
 *
 */
#include <deque>

#ifndef POOLVECTOR_HPP
#define POOLVECTOR_HPP

template <typename T>
class PoolDeque {
 private:
  std::deque<T> pool;
  int maxSize;

 public:
  PoolDeque(int maxSize);
  PoolDeque() {}
  ~PoolDeque();

  void enQueue(T value);
  // 重载下标运算符
  T& operator[](int index);
  bool isEmpty() const { return this->pool.empty(); }
  bool isFull() const { return this->pool.size() == this->maxSize; }
  int size() const { return this->pool.size(); }
  PoolDeque(const PoolDeque& other)
      : pool(other.pool), maxSize(other.maxSize) {}
  PoolDeque& operator=(const PoolDeque& other) {
    if (this != &other) {
      pool = other.pool;
      maxSize = other.maxSize;
    }
    return *this;
  }
};

#include "PoolDeque.tpp"

#endif  // !POOLVECTOR_HPP
