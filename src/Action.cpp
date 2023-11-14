#include "Action.hpp"

Action::Action(std::string name, int id) {
  this->name = name;
  this->id = id;
}

Action::Action(const Action& action): name(action.name), id(action.id) {}

Action::Action() {}

Action::~Action() {}