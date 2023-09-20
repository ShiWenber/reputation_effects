#ifndef STRATEGY_HPP
#define STRATEGY_HPP

#include <string>

class Strategy {
 private:
  std::string name;  //< 行动名称
  int id;            //< 行动id
 public:
  Strategy(std::string name, int id);
  Strategy();
  ~Strategy();

  bool operator<(const Strategy &action) const { return this->id < action.id; }

  std::string getName() const { return this->name; }
  void setName(const std::string &name) { this->name = name; }

  int getId() const { return this->id; }
  void setId(int id) { this->id = id; }

};

#endif  // STRATEGY_HPP
