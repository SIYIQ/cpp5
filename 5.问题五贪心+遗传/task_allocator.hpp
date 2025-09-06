#pragma once

#include <unordered_map>
#include <vector>
#include <string>
#include <Eigen/Dense>
#include "config.hpp"

using Vector3d = Eigen::Vector3d;

namespace TaskAllocator {

/**
 * @brief 计算无人机拦截导弹的成本，以"最小反应时间"为标准
 * 
 * @param uav_spec 无人机配置
 * @param missile_spec 导弹配置  
 * @return double 拦截成本（反应时间）
 */
double calculate_engagement_time_cost(
    const Config::UAVInitial& uav_spec,
    const Config::MissileInitial& missile_spec
);

/**
 * @brief 根据威胁权重，使用贪心策略动态分配无人机
 * 
 * @param threat_weights 导弹ID到威胁权重的映射
 * @return std::unordered_map<std::string, std::unordered_map<std::string, int>> 
 *         导弹ID到{无人机ID: 弹药数量}的分配方案
 */
std::unordered_map<std::string, std::unordered_map<std::string, int>> assign_tasks_by_threat(
    const std::unordered_map<std::string, double>& threat_weights
);

/**
 * @brief 打印任务分配结果
 * 
 * @param assignments 分配方案
 * @param threat_weights 威胁权重（用于显示）
 */
void print_assignment_results(
    const std::unordered_map<std::string, std::unordered_map<std::string, int>>& assignments,
    const std::unordered_map<std::string, double>& threat_weights
);

} // namespace TaskAllocator
