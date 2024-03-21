/**
 * @file MyRandom.hpp
 * @author ShiWenber (1210169842@qq.com)
 * @brief this file is used to define the most used random number generator. In order to ensure that the paper is reproduced accurately, the random number generator will use a global seed like the python global random seed. This is a singleton utils class.
 * @version 0.1
 * @date 2024-03-21
 * 
 * @copyright Copyright (c) 2024
 * 
 */


#ifndef MYRANDOM_HPP
#define MYRANDOM_HPP

#include <random>
#include <chrono>

/**
 * @brief this class is used to define the most used random number generator.
 * Including the global seed and the random number generator.
 * random number generator:
 * 1. getProbability: get a random number in the range of [0, 1].
 * 
 * this is a singleton utils class without thread safe.
 */
class MyRandom
{
private:
    std::mt19937 gen;  //< double Random number generator
    std::uniform_real_distribution<double> prob_dis{0, 1}; //< probability distribution
    const unsigned seed; //< global seed
    MyRandom(
        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count()
    ) : seed(seed), gen(seed) {} // generate random numbers with time-based seed
    MyRandom(MyRandom const&) = delete; // prohibit copy constructor
    MyRandom& operator=(MyRandom const&) = delete; // prohibit copy assignment
public:
    static MyRandom& getInstance(unsigned seed = std::chrono::system_clock::now().time_since_epoch().count()) {
        // static MyRandom instance(seed); // static variable if you use the static, the instance will be singleton among all threads.
        thread_local MyRandom instance(seed); // thread_local will allow you to have a singleton instance for each thread.
        return instance;
    }

    double getProbability();
    double getCurrentSeed() { return this->seed; }
};
    
#endif // !MYRANDOM_HPP