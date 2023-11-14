#include "PoolDeque.hpp"

template <typename T>
PoolDeque<T>::PoolDeque(int maxSize) {
    this->maxSize = maxSize;
}


template <typename T>
PoolDeque<T>::~PoolDeque() {}

template <typename T>
void PoolDeque<T>::enQueue(T value) {
    if (this->isFull()) {
        this->pool.pop_front();
    }
    this->pool.push_back(value);
}

template <typename T>
T& PoolDeque<T>::operator[](int index) {
    return this->pool[index];
}

