/**
 * @file Bimap.hpp
 * @author 
 * @brief bi-directional map
 * @version 0.1
 * @date 2023-11-01
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef BIMAP_HPP
#define BIMAP_HPP


#include <iostream>
#include <string>
#include <unordered_map>

template <typename KeyType, typename ValueType>
class BiMap {
 private:
  std::unordered_map<KeyType, ValueType> forwardMap;
  std::unordered_map<ValueType, KeyType> reverseMap;

 public:
  BiMap() {}
  BiMap(const BiMap& other)
      : forwardMap(other.forwardMap), reverseMap(other.reverseMap) {}
  ~BiMap() {}


  void insert(const KeyType& key, const ValueType& value) {
    forwardMap[key] = value;
    reverseMap[value] = key;
  }

  ValueType at(const KeyType& key) const { return forwardMap.at(key); }

  KeyType getKey(const ValueType& value) const { return reverseMap.at(value); }

  int size() const { return forwardMap.size(); }

  void print() const;

};


#include "BiMap.tpp"

#endif // !BIMAP_HPP