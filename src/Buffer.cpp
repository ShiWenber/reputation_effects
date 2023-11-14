#include "Buffer.hpp"
#include "PoolDeque.hpp"


Buffer::Buffer(int population, int capacity) {
    this->buffer = std::vector<PoolDeque<Transition>>(population);
    for (int i = 0; i < population; i++)
    {
        this->buffer[i] = PoolDeque<Transition>(capacity);
    }
}

Buffer::~Buffer() {}

void Buffer::add(int index, Transition transition) {
    this->buffer[index].enQueue(transition);
}

std::vector<Transition> Buffer::sample(int index, int batchSize) {
    std::vector<Transition> batch;
    int size = this->buffer[index].size();
    if (size < batchSize) {
        batchSize = size;
    }
    for (int i = 0; i < batchSize; i++) {
        batch.push_back(this->buffer[index][i]);
    }
    return batch;
}
