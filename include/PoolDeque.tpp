#include "PoolDeque.hpp"
#include <chrono>
#include <vector>
#include <algorithm>
#include <cassert>

template <typename T>
PoolDeque<T>::PoolDeque(int maxSize) {
    this->maxSize = maxSize;
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    this->gen = std::mt19937(seed);  //< generate random numbers with time-based seed
}


template <typename T>
PoolDeque<T>::~PoolDeque() {}

template <typename T>
void PoolDeque<T>::enQueue(T const& value) {
    if (this->isFull()) {
        this->pool.pop_front();
    }
    this->pool.push_back(value);
}

template <typename T>
T& PoolDeque<T>::operator[](int index) {
    return this->pool[index];
}


/**
 * @brief sample batchSize elements from the pool
 * 
 * @tparam T 
 * @param batchSize 
 * @return std::vector<T> 
 */
template <typename T>
std::vector<T> PoolDeque<T>::sample(int batchSize) {
    assert(batchSize <= this->size());
    std::vector<T> batch;
    // to use this sampling algorithm, you need C++17 !!
    std::sample(this->pool.begin(), this->pool.end(), std::back_inserter(batch), batchSize, this->gen);
    return batch;
}

