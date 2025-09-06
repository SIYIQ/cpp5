#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>

#include "config.hpp"
#include "optimizer.hpp"
#include "utils.hpp"

int main() {
    // --- 步骤 0: 定义问题空间 ---
    std::vector<std::string> all_uav_ids;
    const auto& uav_specs = get_uavs_initial();
    for(const auto& [id, spec] : uav_specs) all_uav_ids.push_back(id);
    std::sort(all_uav_ids.begin(), all_uav_ids.end());

    std::vector<std::string> all_missile_ids;
    const auto& missile_specs = get_missiles_initial();
    for(const auto& [id, spec] : missile_specs) all_missile_ids.push_back(id);
    std::sort(all_missile_ids.begin(), all_missile_ids.end());

    std::map<std::string, int> uav_grenade_counts;
    for(const auto& id : all_uav_ids) uav_grenade_counts[id] = 3;

    std::cout << std::string(70, '=') << std::endl;
    std::cout << "      Global Collaborative Strategy Optimization (Problem 5 C++)" << std::endl;
    std::cout << "  Method: Single Global Optimizer (NLopt MLSL+LDS)" << std::endl;
    std::cout << std::string(70, '=') << std::endl;

    // --- 步骤 1: 定义威胁权重 ---
    std::cout << "\n--- Defining Threat Weights ---" << std::endl;
    double num_missiles = all_missile_ids.size();
    std::map<std::string, double> threat_weights;
    for(const auto& id : all_missile_ids) {
        threat_weights[id] = 1.0 / num_missiles;
        std::cout << "  - Missile " << id << " threat weight (equal): " << std::fixed << std::setprecision(3) << threat_weights[id] << std::endl;
    }
    std::cout << "---------------------------" << std::endl;

    // --- 步骤 2: 构建全局优化问题的边界 (Bounds) ---
    std::vector<double> lower_bounds;
    std::vector<double> upper_bounds;

    for (const auto& uav_id : all_uav_ids) {
        int num_grenades = uav_grenade_counts.at(uav_id);

        // Flight strategy bounds (speed, angle)
        lower_bounds.push_back(UAV_SPEED_MIN); upper_bounds.push_back(UAV_SPEED_MAX);
        lower_bounds.push_back(0.0); upper_bounds.push_back(2 * M_PI);

        // Grenade strategy bounds
        for (int i = 0; i < num_grenades; ++i) {
            if (i == 0) {
                lower_bounds.push_back(0.1); upper_bounds.push_back(30.0); // t_deploy1
            } else {
                lower_bounds.push_back(GRENADE_INTERVAL); upper_bounds.push_back(15.0); // delta_t
            }
            lower_bounds.push_back(0.1); upper_bounds.push_back(20.0); // t_fuse
            lower_bounds.push_back(0.0); upper_bounds.push_back(1.0);  // target_selector
        }
    }

    // --- 步骤 3: 实例化并运行全局优化器 ---
    GlobalOptimizer optimizer(
        all_uav_ids,
        all_missile_ids,
        threat_weights,
        uav_grenade_counts
    );

    int D = lower_bounds.size();
    std::cout << "\nGlobal optimization dimension: " << D << std::endl;
    int pop_size = 20 * D;
    int max_eval = 500; // Increased for a more thorough search

    auto start_time = std::chrono::high_resolution_clock::now();
    auto [optimal_strategy, max_score] = optimizer.solve(lower_bounds, upper_bounds, pop_size, max_eval);
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;

    // --- 步骤 4: 展示和保存结果 ---
    std::cout << std::string(70, '=') << std::endl;
    std::cout << "\nOptimization finished in: " << std::fixed << std::setprecision(2) << elapsed.count() << " seconds." << std::endl;
    std::cout << "Found optimal strategy with weighted score: " << std::fixed << std::setprecision(4) << max_score << std::endl;
    std::cout << std::string(70, '=') << std::endl;

    std::cout << "\n--- Global Optimal Collaborative Strategy Details ---" << std::endl;
    for (const auto& [uav_id, uav_strat] : optimal_strategy) {
        std::cout << "  UAV: " << uav_id << std::endl;
        std::cout << "    Flight Strategy: speed = " << uav_strat.speed << " m/s, angle = " << uav_strat.angle * 180.0 / M_PI << " degrees" << std::endl;
        int g_idx = 1;
        for (const auto& g : uav_strat.grenades) {
            std::cout << "    - Grenade " << g_idx++ << ": t_deploy=" << g.t_deploy << "s, t_fuse=" << g.t_fuse << "s -> Target: " << g.target_missile << std::endl;
        }
    }
    std::cout << "--------------------------------------------------" << std::endl;

    save_global_strategy_to_csv("result_global_optimal.csv", optimal_strategy);

    return 0;
}
