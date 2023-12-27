#include "Action.hpp"

Action::Action(std::string name, int id) {
  this->name = name;
  this->id = id;
}

bool Action::operator==(const Action& action) const {
  return this->id == action.id && this->name == action.name;
}

Action::Action(const Action& action): name(action.name), id(action.id) {}

Action::Action() {}

Action::~Action() {}