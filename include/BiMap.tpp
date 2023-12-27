#include "BiMap.hpp"

template <typename KeyType, typename ValueType>
void BiMap<KeyType, ValueType>::print() const {
  std::cout << "{";
  for (auto& kv : forwardMap) {
    std::cout << kv.first << ":" << kv.second << ", ";
  }
  std::cout << "}" << "\n";
  std::cout << "{";
  for (auto& kv : reverseMap) {
    std::cout << kv.first << ":" << kv.second << ", ";
  }
  std::cout << "}" << "\n";
}
