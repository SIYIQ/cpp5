#pragma once

#include "optimizer.hpp"
#include <unordered_map>
#include <string>

namespace Problem5 {

/**
 * @brief 问题5的子优化器，继承自通用遮蔽优化器
 */
class Problem5SubOptimizer : public Optimizer::ObscurationOptimizer {
public:
    /**
     * @brief 构造函数
     * 
     * @param missile_id 目标导弹ID
     * @param uav_assignments 无人机分配方案 {无人机ID: 弹药数量}
     */
    Problem5SubOptimizer(const std::string& missile_id, 
                         const std::unordered_map<std::string, int>& uav_assignments);

protected:
    /**
     * @brief 解析决策变量，将向量转换为策略结构
     * 
     * @param decision_variables 决策变量向量
     * @return Optimizer::StrategyMap 解析后的策略映射
     */
    Optimizer::StrategyMap parse_decision_variables(const Eigen::VectorXd& decision_variables) override;
};

/**
 * @brief 问题5的主求解函数
 * 
 * @return int 程序退出代码
 */
int solve_problem_5();

/**
 * @brief 打印策略详情
 * 
 * @param missile_id 导弹ID
 * @param strategy 策略详情
 * @param obscuration_time 遮蔽时间
 * @param weight 威胁权重
 */
void print_strategy_details(const std::string& missile_id,
                           const Optimizer::StrategyMap& strategy,
                           double obscuration_time,
                           double weight);

} // namespace Problem5
