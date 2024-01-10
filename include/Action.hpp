#ifndef ACTION_HPP
#define ACTION_HPP

#include <string>

class Action {
 private:
  std::string name;
  int id;
 public:
  Action(std::string name, int id);
  Action();
  ~Action();

  bool operator<(const Action &action) const { return this->id < action.id; }

  std::string getName() const { return this->name; }
  void setName(const std::string &name) { this->name = name; }

  int getId() const { return this->id; }
  void setId(int id) { this->id = id; }

};

#endif  // ACTION_HPP
