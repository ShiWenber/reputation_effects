/**
 * @file Transition.hpp
 * @author ShiWenber (1210169842@qq.com)
 * @brief Markov Transition (state, action, reward, next_state)
 * @version 0.1
 * @date 2023-11-02
 *
 * @copyright Copyright (c) 2023
 *
 */


#ifndef TRANSITION_HPP
#define TRANSITION_HPP

#include <string>

#include "Action.hpp"

class Transition {
 private:
  std::string state;
  Action action;
  double reward;
  std::string nextState;

 public:
  Transition(std::string state, Action action, double reward,
             std::string nextState);
  ~Transition();

  std::string getState() const { return this->state; }

  Action getAction() const { return this->action; }

  double getReward() const { return this->reward; }

  std::string getNextState() const { return this->nextState; }
};

#endif /*TRANSITION_HPP*/