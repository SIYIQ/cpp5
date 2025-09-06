#include "task_allocator.hpp"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <set>
#include <cmath>

namespace TaskAllocator {

double calculate_engagement_time_cost(
    const Config::UAVInitial& uav_spec,
    const Config::MissileInitial& missile_spec) {
    
    Vector3d uav_pos = uav_spec.pos;
    Vector3d missile_start_pos = missile_spec.pos;
    Vector3d missile_target_pos = missile_spec.target;
    
    // 计算拦截点（导弹轨迹的1/3处）
    Vector3d intercept_point = missile_start_pos + (missile_target_pos - missile_start_pos) / 3.0;
    
    // 计算无人机到拦截点的距离和时间
    double distance_to_intercept = (uav_pos - intercept_point).norm();
    double time_to_intercept = distance_to_intercept / Config::UAV_SPEED_MAX;
    
    return time_to_intercept;
}

std::unordered_map<std::string, std::unordered_map<std::string, int>> assign_tasks_by_threat(
    const std::unordered_map<std::string, double>& threat_weights) {
    
    std::vector<std::string> uav_ids;
    std::vector<std::string> missile_ids;
    
    // 收集所有ID
    for (const auto& [uav_id, _] : Config::UAVS_INITIAL) {
        uav_ids.push_back(uav_id);
    }
    for (const auto& [missile_id, _] : Config::MISSILES_INITIAL) {
        missile_ids.push_back(missile_id);
    }
    
    int num_uavs = uav_ids.size();
    
    std::cout << "\n--- 正在根据威胁权重计算资源需求 ---" << std::endl;
    
    // 1. 根据权重决定每个导弹应分配的无人机数量
    std::unordered_map<std::string, int> allocation_requirements;
    
    // 初始分配：基于权重按比例分配
    for (const auto& [missile_id, weight] : threat_weights) {
        allocation_requirements[missile_id] = static_cast<int>(std::round(weight * num_uavs));
    }
    
    // 调整总数以匹配可用无人机数量
    int current_total = 0;
    for (const auto& [_, num] : allocation_requirements) {
        current_total += num;
    }
    
    // 如果总数不足，补充给威胁权重最高的导弹
    while (current_total < num_uavs) {
        auto max_threat_it = std::max_element(
            threat_weights.begin(), threat_weights.end(),
            [](const auto& a, const auto& b) { return a.second < b.second; }
        );
        allocation_requirements[max_threat_it->first]++;
        current_total++;
    }
    
    // 如果总数过多，从威胁权重最低的导弹减少
    while (current_total > num_uavs) {
        auto min_threat_it = std::min_element(
            threat_weights.begin(), threat_weights.end(),
            [](const auto& a, const auto& b) { return a.second < b.second; }
        );
        if (allocation_requirements[min_threat_it->first] > 0) {
            allocation_requirements[min_threat_it->first]--;
            current_total--;
        } else {
            break; // 避免无限循环
        }
    }
    
    // 打印分配需求
    for (const auto& [missile_id, num] : allocation_requirements) {
        auto weight_it = threat_weights.find(missile_id);
        double weight = (weight_it != threat_weights.end()) ? weight_it->second : 0.0;
        std::cout << "  导弹 " << missile_id 
                  << " (权重 " << std::fixed << std::setprecision(2) << weight 
                  << ") -> 分配 " << num << " 架无人机" << std::endl;
    }
    std::cout << "------------------------------------" << std::endl;
    
    // 2. 基于成本排序的贪心分配
    std::unordered_map<std::string, std::unordered_map<std::string, double>> uav_missile_costs;
    
    for (const auto& uav_id : uav_ids) {
        for (const auto& missile_id : missile_ids) {
            const auto& uav_spec = Config::UAVS_INITIAL.at(uav_id);
            const auto& missile_spec = Config::MISSILES_INITIAL.at(missile_id);
            uav_missile_costs[uav_id][missile_id] = calculate_engagement_time_cost(uav_spec, missile_spec);
        }
    }
    
    // 执行分配
    std::unordered_map<std::string, std::unordered_map<std::string, int>> assignments;
    std::set<std::string> assigned_uavs;
    
    // 初始化assignments
    for (const auto& missile_id : missile_ids) {
        assignments[missile_id] = {};
    }
    
    // 按威胁权重从高到低分配
    std::vector<std::pair<std::string, double>> sorted_threats(threat_weights.begin(), threat_weights.end());
    std::sort(sorted_threats.begin(), sorted_threats.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    for (const auto& [missile_id, _] : sorted_threats) {
        int num_needed = allocation_requirements[missile_id];
        if (num_needed == 0) continue;
        
        // 收集候选无人机并按成本排序
        std::vector<std::pair<std::string, double>> candidates;
        for (const auto& uav_id : uav_ids) {
            if (assigned_uavs.find(uav_id) == assigned_uavs.end()) {
                candidates.emplace_back(uav_id, uav_missile_costs[uav_id][missile_id]);
            }
        }
        
        std::sort(candidates.begin(), candidates.end(),
                  [](const auto& a, const auto& b) { return a.second < b.second; });
        
        // 分配最优的无人机
        int assigned_count = 0;
        for (const auto& [uav_id, _] : candidates) {
            if (assigned_count >= num_needed) break;
            
            assignments[missile_id][uav_id] = 3; // 每架无人机分配3枚弹药
            assigned_uavs.insert(uav_id);
            assigned_count++;
        }
    }
    
    return assignments;
}

void print_assignment_results(
    const std::unordered_map<std::string, std::unordered_map<std::string, int>>& assignments,
    const std::unordered_map<std::string, double>& threat_weights) {
    
    std::cout << "\n--- 最终任务分配方案 (基于威胁权重和成本) ---" << std::endl;
    
    for (const auto& [missile_id, uav_alloc] : assignments) {
        if (uav_alloc.empty()) {
            std::cout << "  导弹 " << missile_id << " 未分配到拦截资源。" << std::endl;
            continue;
        }
        
        std::vector<std::string> uav_list;
        for (const auto& [uav_id, _] : uav_alloc) {
            uav_list.push_back(uav_id);
        }
        
        std::cout << "  导弹 " << missile_id << " 由无人机 ";
        for (size_t i = 0; i < uav_list.size(); ++i) {
            std::cout << uav_list[i];
            if (i < uav_list.size() - 1) std::cout << ", ";
        }
        std::cout << " 进行拦截。" << std::endl;
    }
    
    std::cout << "------------------------------------------------" << std::endl;
}

} // namespace TaskAllocator
