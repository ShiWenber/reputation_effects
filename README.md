# game theory

## requirements

os: linux

- [cmake](https://cmake.org/cmake/help/latest/guide/tutorial/index.html)
- [vcpkg](https://vcpkg.io/en/)

we use vcpkg to manage our dependencies, so you need to install vcpkg in your environment and make sure that `vcpkg` in your path environment variable. 
For some libraries can't install by `vcpkg`, we use `git submodule` to manage them (they are in `./third_party`).
our `git submodule` use the `ssh` protocol, so you need to set your `ssh` key in your github account first.
if you encounter web connection problem, we also recommand you to use `ssh` url `git@github.com:<path>` for `git submodule`
As for vcpkg, we don't know how to change its downloading protocol from `https` to `ssh`, so when you encounter web connection problem, you can only wait for it to finish or just try again.

we use c++ to simulate the game, use python to analyze data, draw pictures and use python to automatically derive formulas.

The python code is all put into the `.ipynb` file by us, and the running results and formula derivation process are attached, so you may also need to install `jupyter notebook` to render the `.ipynb` file.

- [jupyter notebook](https://jupyter.org/install)

Or you can use IDE that support rendering `.ipynb` file, such as `pycharm`, `vscode` and so on.

## python virtual environment

the python packages needed are in `requirements.txt`.

## C++ project build

<!-- TODO: this needs test! -->

It is recommand to use IDE to load the cmake project.

or use command line:

```bash
cd <project root>
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=<vcpkg root>/scripts/buildsystems/vcpkg.cmake -DEABLE_ASSERTS=OFF # -DEABLE_ASSERTS=OFF will disable asserts and speed up the program by enabling compiler optimization flags -O3
make
```

For loading the config file in `./norm`, `./strategy` and so on, you may need to move the exe file to the root of the project before running it.

---

## 混合策略-纯策略

纯策略是指在博弈中，玩家的策略是确定的，不会随机变化的策略。例如在石头剪刀布中，玩家的策略是固定的，不会随机变化。纯策略就是玩家策略集中的某个策略。

## 支持情况

当前该库仅支持双人博弈的试验，没有考虑多人博弈的情况。

当前库为CPU版本，缺少多进程支持

## test

```Cpp
/**
 * @file rock-paper-scissors.cpp
 * @author ShiWenber (1210169842@qq.com)
 * @brief 石头剪刀布博弈模拟
 * @version 0.1
 * @date 2023-09-12
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <iostream>
// 导入字典类型
#include <fmt/ranges.h>

#include <filesystem>
#include <fstream>
#include <map>
#include <string>

#include "Action.hpp"
#include "Norm.hpp"
#include "PayoffMatrix.hpp"
#include "Player.hpp"
#include "Strategy.hpp"

#define REPUTATION_STR "reputation"

using namespace std;

int main() {
  // 博弈参数
  int stepNum = 100;
  cout << "---" << endl;

  // 加载norm
  Norm norm("./norm.csv");
  fmt::print("norm: {}\n", norm.getNormTableStr());

  // 加载payoffMatrix
  cout << "---------->>" << endl;
  PayoffMatrix payoffMatrix_g("./GPayoffMatrix.csv");
  fmt::print("payoffMatrix_g: {}\n", payoffMatrix_g.getPayoffMatrixStr());
  // 输出需要赋值的所有变量
  fmt::print("vars: {}\n", payoffMatrix_g.getVars());
  cout << "after assign:" << endl;

  // vars: {b: 0, beta: 0, c: 0, gamma: 0, lambda: 0, }
  // 赋值后
  payoffMatrix_g.updateVar("b", 1);
  payoffMatrix_g.updateVar("beta", 1);
  payoffMatrix_g.updateVar("c", 1);
  payoffMatrix_g.updateVar("gamma", 1);
  //   payoffMatrix_g.updateVar("lambda", 1);
  fmt::print("vars: {}\n", payoffMatrix_g.getVars());

  payoffMatrix_g.evalPayoffMatrix();
  cout << "after eval:" << endl;
  for (int r = 0; r < payoffMatrix_g.getRowNum(); r++) {
    for (int c = 0; c < payoffMatrix_g.getColNum(); c++) {
      for (int p = 0; p < payoffMatrix_g.getPlayerNum(); p++) {
        cout << payoffMatrix_g.getPayoffMatrix()[r][c][p] << ",";
      }
      cout << "\t";
    }
    cout << endl;
  }
  cout << "---------<<" << endl;

  cout << "--------->>" << endl;
  PayoffMatrix payoffMatrix_b("./BPayoffMatrix.csv");
  fmt::print("payoffMatrix_b: {}\n", payoffMatrix_b.getPayoffMatrixStr());
  // 输出需要赋值的所有变量
  fmt::print("payoffMatrix_b vars: {}\n", payoffMatrix_b.getVars());

  // vars: {b: 0, beta: 0, c: 0, gamma: 0, lambda: 0, }
  // 赋值后
  payoffMatrix_b.updateVar("b", 1);
  payoffMatrix_b.updateVar("beta", 1);
  payoffMatrix_b.updateVar("c", 1);
  payoffMatrix_b.updateVar("gamma", 1);
  //   payoffMatrix_b.updateVar("lambda", 1);
  cout << "after assign:" << endl;
  fmt::print("payoffMatrix_b vars: {}\n", payoffMatrix_b.getVars());

  payoffMatrix_b.evalPayoffMatrix();
  cout << "after eval:" << endl;
  for (int r = 0; r < payoffMatrix_b.getRowNum(); r++) {
    for (int c = 0; c < payoffMatrix_b.getColNum(); c++) {
      for (int p = 0; p < payoffMatrix_b.getPlayerNum(); p++) {
        cout << payoffMatrix_b.getPayoffMatrix()[r][c][p] << ",";
      }
      cout << "\t";
    }
    cout << endl;
  }

  cout << "---------<<" << endl;

  // 设置公共信息
  //   Player::addCommonInfo("lambda", 1);
  fmt::print("commonInfo: {}\n", Player::getCommonInfo());

  // 初始化两个博弈玩家
  vector<Action> donorActions;
  donorActions.push_back(Action("C", 0));
  donorActions.push_back(Action("D", 1));
  Player donor("donor", 0, donorActions);
  vector<Strategy> donorStrategies;
  donorStrategies.push_back(Strategy("C", 0));
  donorStrategies.push_back(Strategy("OC", 1));
  donorStrategies.push_back(Strategy("OD", 2));
  donorStrategies.push_back(Strategy("D", 3));
  donor.setStrategies(donorStrategies);
  donor.loadStrategy("./strategy");
  fmt::print("donorStrategies: {}\n", donor.getStrategyTables());
  // TODO: 设置初始策略
  donor.setStrategy("C");

  vector<Action> recipientActions;
  recipientActions.push_back(Action("C", 0));
  recipientActions.push_back(Action("D", 1));
  Player recipient("recipient", 0, recipientActions);
  vector<Strategy> recipientStrategies;
  recipientStrategies.push_back(Strategy("NR", 0));
  recipientStrategies.push_back(Strategy("SR", 1));
  recipientStrategies.push_back(Strategy("AR", 2));
  recipientStrategies.push_back(Strategy("UR", 3));
  recipient.setStrategies(recipientStrategies);

  recipient.loadStrategy("./strategy");
  fmt::print("recipientStrategies: {}\n", recipient.getStrategyTables());
  // TODO: 设置初始化策略
  recipient.setStrategy("NR");

  // 给声誉一个 0-1 的随机整数
  // 时间随机种子
  unsigned seed = chrono::system_clock::now().time_since_epoch().count();
  default_random_engine gen(seed);
  uniform_int_distribution<int> dis(0, 1);
  recipient.addVar(REPUTATION_STR, dis(gen));
  fmt::print("recipient vars: {}\n", recipient.getVars());

  for (int step = 0; step < stepNum; step++) {
    // 博弈测试，一轮
    // 设置donor和recipient为随机策略
    // 设置随机数0-3
    uniform_int_distribution<int> dis2(0, 3);
    donor.setStrategy(donorStrategies.at(dis2(gen)));
    recipient.setStrategy(recipientStrategies.at(dis2(gen)));

    cout << endl << "-------------------- step " << step << endl;
    // 第一阶段 donor 行动
    Action donorAction = donor.donate(
        std::to_string((int)recipient.getVarValue(REPUTATION_STR)));
    fmt::print("donorStrategy:{0}, donorAction: {1}\n",
               donor.getStrategy().getName(), donorAction.getName());
    // 第二阶段 recipient 行动，记录本轮声望
    Action recipientAction = recipient.reward(donorAction.getName());
    double currentReputation = recipient.getVarValue(REPUTATION_STR);
    fmt::print("recipientStrategy:{0}, recipientAction: {1}\n",
               recipient.getStrategy().getName(), recipientAction.getName());
    // 第三阶段 更新recipient 的声誉
    double newReputation = norm.getReputation(donorAction, recipientAction);
    fmt::print("reputation : {} -> ", currentReputation);
    fmt::print("new: {} \n", newReputation);
    recipient.updateVar(REPUTATION_STR, newReputation);
    // 第四阶段 结算双方收益
    double donorPayoff, recipientPayoff;
    if (currentReputation == 1) {
      donorPayoff = payoffMatrix_g.getPayoff(donor.getStrategy(),
                                             recipient.getStrategy())[0];
      recipientPayoff = payoffMatrix_g.getPayoff(donor.getStrategy(),
                                                 recipient.getStrategy())[1];
    } else if (currentReputation == 0) {
      donorPayoff = payoffMatrix_b.getPayoff(donor.getStrategy(),
                                             recipient.getStrategy())[0];
      recipientPayoff = payoffMatrix_b.getPayoff(donor.getStrategy(),
                                                 recipient.getStrategy())[1];
    }
    donor.updateScore(donorPayoff);
    recipient.updateScore(recipientPayoff);
    fmt::print("donor:{0}, recipient:{1}", donorPayoff, recipientPayoff);
  }

  return 0;
}
```

主循环结构和并行分析

```Cpp
// 第一个循环是每对博弈者独立交互，可并行，内部都是简单操作，没有循环，并行可能副作用 并行模块1
for (int i = 0; i < population; i++) {

}
// 会将 deltaScore 存入每个博弈者

// 第二个循环是基于前一个循环记录的deltaScore来计算每个人的策略如何转变，内部有大循环，该循环建议并行 并行模块2
for (int i = 0; i < population; i++) {

}
// 会将每个博弈者如何转变策略记录下来

// 遍历每个策略的集合并应用转变，donor和recipient相互独立且内部有大循环，建议并行 并行模块3


// 模块1-deltaScore->模块2-转变->模块3 有先后顺序的依赖，模块间不允许并行
```

当程序因为异常停止，如果输出异常信息为 no alter strategy 可能表示所有的博弈者都采用了同一策略，已经没有策略可以转变了

## 我还需要一个东西帮我自动推导不同norm下的matrix payoff matrix