#include "MyRandom.hpp"

double MyRandom::getProbability() {
    return this->prob_dis(this->gen);
}