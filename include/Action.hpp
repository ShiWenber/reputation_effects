#ifndef ACTION_HPP
#define ACTION_HPP

#include <string>

class Action {
 private:
  std::string name;  //< 行动名称
  int id;            //< 行动id
  double payoff; //< 行动收益
 public:
  Action(std::string name, int id);
  Action(const Action &action);
  Action();
  ~Action();

  bool operator<(const Action &action) const { return this->id < action.id; }
  bool operator==(const Action &action) const;

  std::string getName() const { return this->name; }
  void setName(const std::string &name) { this->name = name; }

  int getId() const { return this->id; }
  void setId(int id) { this->id = id; }

};

#endif  // ACTION_HPP
