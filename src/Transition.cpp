#include "Transition.hpp"

Transition::Transition(std::string state, Action action, double reward,
                       std::string nextState)
    : action(action), state(state), reward(reward), nextState(nextState) {}

Transition::~Transition() {}
