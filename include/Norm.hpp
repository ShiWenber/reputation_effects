#ifndef NORM_HPP
#define NORM_HPP

#include <unordered_map>
#include <vector>
#include <random>

#include "Action.hpp"

class Norm
{
private:
    std::unordered_map<std::string, double> normFunc; //< update reputation using discrete function
    std::vector<std::vector<std::string>> normTableStr; //< Update the reputation using discrete function
    std::mt19937 gen;  //< double Random number generator
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