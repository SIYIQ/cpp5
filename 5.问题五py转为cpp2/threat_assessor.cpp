#include "threat_assessor.hpp"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <vector>

namespace ThreatAssessor {

double calculate_time_to_impact(const std::string& missile_id) {
    auto it = Config::MISSILES_INITIAL.find(missile_id);
    if (it == Config::MISSILES_INITIAL.end()) {
        return 1000.0; // 默认很大的时间
    }
    
    const auto& missile_spec = it->second;
    Vector3d direction = missile_spec.target - missile_spec.pos;
    double distance = direction.norm();
    return distance / missile_spec.speed;
}

double calculate_criticality(const std::string& missile_id) {
    auto it = Config::MISSILES_INITIAL.find(missile_id);
    if (it == Config::MISSILES_INITIAL.end()) {
        return 0.5; // 默认中等关键性
    }
    
    const auto& missile_spec = it->second;
    
    // 基于初始位置的关键性：距离目标越近，关键性越高
    double distance_to_target = (missile_spec.pos - missile_spec.target).norm();
    
    // 基于速度的关键性：速度越快，关键性越高
    double speed_factor = missile_spec.speed / 400.0; // 归一化到400m/s
    
    // 基于高度的关键性：高度适中的导弹更难拦截
    double altitude = missile_spec.pos[2];
    double altitude_factor = 1.0 - std::abs(altitude - 2000.0) / 2000.0; // 2000m为最优高度
    
    // 综合评分
    double distance_score = std::max(0.0, 1.0 - distance_to_target / 25000.0);
    double speed_score = std::min(1.0, speed_factor);
    double altitude_score = std::max(0.0, altitude_factor);
    
    return (distance_score * 0.5 + speed_score * 0.3 + altitude_score * 0.2);
}

double calculate_difficulty(const std::string& missile_id) {
    auto it = Config::MISSILES_INITIAL.find(missile_id);
    if (it == Config::MISSILES_INITIAL.end()) {
        return 0.5; // 默认中等难度
    }
    
    const auto& missile_spec = it->second;
    
    // 基于初始位置偏离程度的难度
    Vector3d target_center = Config::TRUE_TARGET_SPECS.center_bottom;
    Vector3d missile_pos = missile_spec.pos;
    
    // Y方向偏离（侧向偏离）
    double lateral_deviation = std::abs(missile_pos[1] - target_center[1]);
    
    // Z方向偏离（高度偏离）  
    double altitude_deviation = std::abs(missile_pos[2] - 2000.0); // 假设2000m为标准高度
    
    // 距离因子
    double distance = (missile_pos - target_center).norm();
    
    // 综合难度评分
    double lateral_score = std::min(1.0, lateral_deviation / 1000.0); // 归一化
    double altitude_score = std::min(1.0, altitude_deviation / 1000.0);
    double distance_score = std::min(1.0, distance / 20000.0);
    
    return (lateral_score * 0.4 + altitude_score * 0.3 + distance_score * 0.3);
}

ThreatMetrics assess_single_missile_threat(
    const std::string& missile_id,
    const FactorWeights& factor_weights) {
    
    double tti = calculate_time_to_impact(missile_id);
    double crit = calculate_criticality(missile_id);
    double diff = calculate_difficulty(missile_id);
    
    // 时间到达威胁：时间越短威胁越高
    double tti_score = 1.0 / (1.0 + tti / 60.0); // 归一化，60秒为参考时间
    
    // 综合威胁评分
    double overall_threat = factor_weights.tti * tti_score + 
                           factor_weights.crit * crit + 
                           factor_weights.diff * diff;
    
    return ThreatMetrics(tti, crit, diff, overall_threat);
}

std::unordered_map<std::string, double> assess_threat_weights(
    const FactorWeights& factor_weights) {
    
    std::unordered_map<std::string, double> threat_weights;
    std::vector<std::pair<std::string, double>> threat_scores;
    
    // 计算每个导弹的威胁评分
    for (const auto& [missile_id, _] : Config::MISSILES_INITIAL) {
        ThreatMetrics metrics = assess_single_missile_threat(missile_id, factor_weights);
        threat_scores.emplace_back(missile_id, metrics.overall_threat);
    }
    
    // 归一化威胁权重，使总和为1.0
    double total_threat = 0.0;
    for (const auto& [_, score] : threat_scores) {
        total_threat += score;
    }
    
    if (total_threat > 0.0) {
        for (const auto& [missile_id, score] : threat_scores) {
            threat_weights[missile_id] = score / total_threat;
        }
    } else {
        // 如果所有威胁评分都为0，平均分配权重
        double equal_weight = 1.0 / Config::MISSILES_INITIAL.size();
        for (const auto& [missile_id, _] : Config::MISSILES_INITIAL) {
            threat_weights[missile_id] = equal_weight;
        }
    }
    
    // 打印威胁评估结果
    std::cout << "\n--- 威胁评估结果 ---" << std::endl;
    for (const auto& [missile_id, weight] : threat_weights) {
        ThreatMetrics metrics = assess_single_missile_threat(missile_id, factor_weights);
        std::cout << "导弹 " << missile_id 
                  << ": 威胁权重=" << std::fixed << std::setprecision(3) << weight
                  << " (TTI=" << std::setprecision(1) << metrics.time_to_impact << "s"
                  << ", 关键性=" << std::setprecision(3) << metrics.criticality
                  << ", 难度=" << metrics.difficulty << ")" << std::endl;
    }
    std::cout << "-------------------" << std::endl;
    
    return threat_weights;
}

} // namespace ThreatAssessor
