#include "Buffer.hpp"
#include "PoolDeque.hpp"
#include <cassert>


Buffer::Buffer(int player_type_num, int population, int capacity) {
    // this->_pool_deques = std::vector<PoolDeque<Transition>>(population);
    // for (int i = 0; i < population; i++)
    // {
    //     this->_pool_deques[i] = PoolDeque<Transition>(capacity);
    // }
    this->_player_type_num = player_type_num;
    this->_pool_deques = std::vector<std::vector<PoolDeque<Transition>>>(player_type_num);
    for (int i = 0; i < player_type_num; i++)
    {
        this->_pool_deques[i] = std::vector<PoolDeque<Transition>>(population);
        for (int j = 0; j < population; j++)
        {
            this->_pool_deques[i][j] = PoolDeque<Transition>(capacity);
        }
    }
}

Buffer::~Buffer() {}

/**
 * @brief add experience to the experience pool of the specified player type
 * 
 * @param index 
 * @param player_type 
 * @param transition 
 */
void Buffer::add(int player_type, int index, Transition const& transition) {
    assert(index < this->_pool_deques[player_type].size());
    assert(player_type < this->_pool_deques.size());
    this->_pool_deques[player_type][index].enQueue(transition);
}

std::vector<Transition> Buffer::sample(int player_type, int index, int batchSize) {
    std::vector<Transition> batch;
    int size = this->_pool_deques[player_type][index].size();
    assert(size >= batchSize);
    // sample from the this->_pool_deques[player_type][index]
    batch = this->_pool_deques[player_type][index].sample(batchSize); 
    
    // for (int i = 0; i < batchSize; i++) {
    //     batch.push_back(this->_pool_deques[player_type][index][i]);
    // }
    return batch;
}
