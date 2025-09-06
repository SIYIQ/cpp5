// solve_problem_5_new.cpp - 全局优化版本
#include "optimizer.hpp"
#include "threat_assessor.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <chrono>
#include <algorithm>
#include <map>

int main() {
    // --- 步骤 0: 定义问题空间 ---
    std::vector<std::string> all_uav_ids;
    const auto& uav_specs = Config::UAVS_INITIAL;
    for(const auto& [id, spec] : uav_specs) all_uav_ids.push_back(id);
    std::sort(all_uav_ids.begin(), all_uav_ids.end());

    std::vector<std::string> all_missile_ids;
    const auto& missile_specs = Config::MISSILES_INITIAL;
    for(const auto& [id, spec] : missile_specs) all_missile_ids.push_back(id);
    std::sort(all_missile_ids.begin(), all_missile_ids.end());

    std::map<std::string, int> uav_grenade_counts;
    for(const auto& id : all_uav_ids) uav_grenade_counts[id] = 3;

    std::cout << std::string(70, '=') << std::endl;
    std::cout << "      全局协同策略优化 (问题五 C++ - 修正版)" << std::endl;
    std::cout << "  方法: 全局优化器 (所有导弹考虑场上所有烟雾云)" << std::endl;
    std::cout << std::string(70, '=') << std::endl;

    // --- 步骤 1: 威胁评估 ---
    std::cout << "\n--- 正在进行威胁评估 ---" << std::endl;
    auto threat_weights = ThreatAssessor::assess_threat_weights();
    std::cout << "---------------------" << std::endl;

    // --- 步骤 2: 构建全局优化问题的边界 (Bounds) ---
    std::vector<Optimizer::Bounds> bounds;
    
    for (const auto& uav_id : all_uav_ids) {
        int num_grenades = uav_grenade_counts[uav_id];
        
        // 飞行策略边界
        bounds.emplace_back(Config::UAV_SPEED_MIN, Config::UAV_SPEED_MAX);
        bounds.emplace_back(0.0, 2.0 * M_PI);
        
        // 弹药策略边界
        for (int i = 0; i < num_grenades; ++i) {
            if (i == 0) {
                bounds.emplace_back(0.1, 30.0);  // t_deploy1
            } else {
                bounds.emplace_back(Config::GRENADE_INTERVAL, 15.0);  // delta_t
            }
            
            bounds.emplace_back(0.1, 20.0);  // t_fuse
            bounds.emplace_back(0.0, 1.0);   // target_selector
        }
    }

    // --- 步骤 3: 实例化并运行全局优化器 ---
    Optimizer::GlobalOptimizer optimizer(
        all_uav_ids,
        all_missile_ids,
        threat_weights,
        uav_grenade_counts
    );

    int D = bounds.size();
    std::cout << "\n全局优化问题维度: " << D << std::endl;
    
    // 调试版本：使用较小的参数进行快速测试
    Optimizer::DESettings settings;
    settings.population_size = 10;        // 固定小种群，快速测试
    settings.max_iterations = 100;       // 减少迭代次数
    settings.tolerance = 0.1;             // 放宽收敛条件
    settings.num_threads = 1;             // 单线程避免输出问题
    
    std::cout << "使用调试参数: 种群=" << settings.population_size 
              << ", 最大迭代=" << settings.max_iterations << std::endl;
    std::cout << "注意: 这是快速测试版本，如需高精度请调大参数" << std::endl;
    
    std::cout << "--- 开始使用差分进化算法求解全局最优策略 ---" << std::endl;
    auto start_time = std::chrono::high_resolution_clock::now();
    auto [optimal_strategy, max_score] = optimizer.solve(bounds, settings);
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;
    std::cout << "--------------------------------------------" << std::endl;

    // --- 步骤 4: 展示和保存结果 ---
    std::cout << "\n优化完成，耗时: " << std::fixed << std::setprecision(2) << elapsed.count() << " 秒。" << std::endl;
    std::cout << "找到的最优策略的加权综合得分: " << std::fixed << std::setprecision(4) << max_score << std::endl;
    std::cout << std::string(70, '=') << std::endl;

    // --- 步骤 5: 计算并展示最优策略的详细结果 ---
    auto obscuration_times = optimizer.calculate_strategy_details(optimal_strategy);
    std::cout << "\n--- 各导弹遮蔽时间分析 ---" << std::endl;
    for(const auto& [missile_id, time] : obscuration_times) {
        auto weight_it = threat_weights.find(missile_id);
        double weight = (weight_it != threat_weights.end()) ? weight_it->second : 0.0;
        std::cout << "  导弹 " << missile_id << ": 遮蔽时间 = " << std::fixed << std::setprecision(2) << time 
                  << "s, 威胁权重 = " << std::setprecision(3) << weight 
                  << ", 加权得分 = " << std::setprecision(3) << (time * weight) << std::endl;
    }
    std::cout << "-----------------------------" << std::endl;

    std::cout << "\n--- 全局最优协同策略详情 ---" << std::endl;
    for (const auto& [uav_id, uav_strat] : optimal_strategy) {
        std::cout << "  UAV: " << uav_id << std::endl;
        std::cout << "    飞行策略: speed = " << std::fixed << std::setprecision(2) << uav_strat.speed 
                  << " m/s, angle = " << (uav_strat.angle * 180.0 / M_PI) << " degrees" << std::endl;
        int g_idx = 1;
        for (const auto& g : uav_strat.grenades) {
            std::cout << "    - 弹药 " << g_idx++ << ": t_deploy=" << g.t_deploy 
                      << "s, t_fuse=" << g.t_fuse << "s -> 目标: " << g.target_missile << std::endl;
        }
    }
    std::cout << "----------------------------------" << std::endl;

    return 0;
}
