#ifndef SOLVE_PROBLEM_4_HPP
#define SOLVE_PROBLEM_4_HPP

#include "optimizer.hpp"
#include <vector>
#include <unordered_map>
#include <string>

/**
 * 问题四优化器：FY1, FY2, FY3 各投放1枚干扰弹，对抗M1。
 */
class Problem4Optimizer : public ObscurationOptimizer {
public:
    Problem4Optimizer();
    
    /**
     * 解析12维决策变量向量。
     * dv = [s1, a1, td1, tf1, s2, a2, td2, tf2, s3, a3, td3, tf3]
     */
    StrategyMap parse_decision_variables(const std::vector<double>& dv) override;
};

/**
 * 问题四主求解函数
 */
void solve_problem_4();

#endif // SOLVE_PROBLEM_4_HPP
