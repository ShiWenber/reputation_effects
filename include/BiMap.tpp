#include "BiMap.hpp"
template <typename KeyType, typename ValueType>
void BiMap<KeyType, ValueType>::print() const {
  fmt::print("forwardMap: {}\n", forwardMap);
  fmt::print("reverseMap: {}\n", reverseMap);
}
