#ifndef SOLVE_PROBLEM_3_HPP
#define SOLVE_PROBLEM_3_HPP

#include "optimizer.hpp"
#include <vector>

/**
 * 问题三优化器：FY1投放3枚干扰弹，对抗M1。
 */
class Problem3Optimizer : public ObscurationOptimizer {
public:
    Problem3Optimizer();
    
    /**
     * 解析8维决策变量向量。
     * dv = [speed, angle, t_d1, t_f1, delta_t2, t_f2, delta_t3, t_f3]
     */
    StrategyMap parse_decision_variables(const std::vector<double>& dv) override;
};

/**
 * 问题三主求解函数
 */
void solve_problem_3();

#endif // SOLVE_PROBLEM_3_HPP
