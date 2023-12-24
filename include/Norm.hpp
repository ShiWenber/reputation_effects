#ifndef NORM_HPP
#define NORM_HPP

#include "Action.hpp"
#include <unordered_map>
#include <vector>

class Norm
{
private:
    std::unordered_map<std::string, double> normFunc; //< 更新声誉使用的离散函数
    std::vector<std::vector<std::string>> normTableStr; //< 更新声誉使用的离散函数

public:
    Norm(/* args */);
    Norm(std::string csvPath);
    ~Norm();
    void loadNormFunc(std::string csvPath);
    std::vector<std::vector<std::string>> getNormTableStr() const { return this->normTableStr; }
    double getReputation(Action const& donorAction, Action const& recipientAction) const;

};

#endif // !NORM_HPP