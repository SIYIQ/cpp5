#pragma once

#include <unordered_map>
#include <string>
#include <Eigen/Dense>
#include "config.hpp"

using Vector3d = Eigen::Vector3d;

namespace ThreatAssessor {

/**
 * @brief 威胁因子权重结构
 */
struct FactorWeights {
    double tti = 0.5;   // time-to-impact 权重
    double crit = 0.3;  // criticality 权重  
    double diff = 0.2;  // difficulty 权重
    
    FactorWeights() = default;
    FactorWeights(double t, double c, double d) : tti(t), crit(c), diff(d) {}
};

/**
 * @brief 威胁评估结果
 */
struct ThreatMetrics {
    double time_to_impact;
    double criticality;
    double difficulty;
    double overall_threat;
    
    ThreatMetrics(double tti, double crit, double diff, double threat)
        : time_to_impact(tti), criticality(crit), difficulty(diff), overall_threat(threat) {}
};

/**
 * @brief 评估单个导弹的威胁权重
 * 
 * @param missile_id 导弹ID
 * @param factor_weights 威胁因子权重
 * @return ThreatMetrics 威胁评估结果
 */
ThreatMetrics assess_single_missile_threat(
    const std::string& missile_id,
    const FactorWeights& factor_weights = FactorWeights()
);

/**
 * @brief 评估所有导弹的威胁权重
 * 
 * @param factor_weights 威胁因子权重
 * @return std::unordered_map<std::string, double> 导弹ID到威胁权重的映射
 */
std::unordered_map<std::string, double> assess_threat_weights(
    const FactorWeights& factor_weights = FactorWeights()
);

/**
 * @brief 计算到达目标的时间
 * 
 * @param missile_id 导弹ID
 * @return double 到达时间（秒）
 */
double calculate_time_to_impact(const std::string& missile_id);

/**
 * @brief 计算关键性评分（基于初始位置和速度）
 * 
 * @param missile_id 导弹ID
 * @return double 关键性评分 [0.0, 1.0]
 */
double calculate_criticality(const std::string& missile_id);

/**
 * @brief 计算拦截难度评分
 * 
 * @param missile_id 导弹ID
 * @return double 难度评分 [0.0, 1.0]
 */
double calculate_difficulty(const std::string& missile_id);

} // namespace ThreatAssessor
