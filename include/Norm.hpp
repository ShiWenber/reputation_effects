#ifndef NORM_HPP
#define NORM_HPP

#include <unordered_map>
#include <vector>
#include <random>

#include "Action.hpp"

class Norm
{
private:
    std::unordered_map<std::string, double> normFunc; //< the discrete function used to update reputation
    std::vector<std::vector<std::string>> normTableStr; //< the discrete function used to update reputation
    std::mt19937 gen;  //< generator for random double
public:
    Norm(/* args */);
    Norm(std::string csvPath);
    ~Norm();
    void loadNormFunc(std::string csvPath);
    std::vector<std::vector<std::string>> getNormTableStr() const { return this->normTableStr; }
    double getReputation(Action const& donorAction, Action const& recipientAction, double const reputation_error_p=0.0);
    double getProbability();

};

#endif // !NORM_HPP