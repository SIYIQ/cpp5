#include "solve_problem_5.hpp"
#include "threat_assessor.hpp"
#include "task_allocator.hpp"
#include "boundary_calculator.hpp"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <cmath>

namespace Problem5 {

Problem5SubOptimizer::Problem5SubOptimizer(
    const std::string& missile_id, 
    const std::unordered_map<std::string, int>& uav_assignments)
    : ObscurationOptimizer(missile_id, uav_assignments) {
}

Optimizer::StrategyMap Problem5SubOptimizer::parse_decision_variables(const Eigen::VectorXd& decision_variables) {
    Optimizer::StrategyMap strategy;
    int dv_index = 0;
    
    // 获取排序后的无人机ID列表，确保解析顺序一致
    std::vector<std::string> sorted_uav_ids;
    for (const auto& [uav_id, _] : uav_assignments_) {
        sorted_uav_ids.push_back(uav_id);
    }
    std::sort(sorted_uav_ids.begin(), sorted_uav_ids.end());
    
    for (const auto& uav_id : sorted_uav_ids) {
        int num_grenades = uav_assignments_.at(uav_id);
        
        // 解析飞行速度和角度
        double speed = decision_variables[dv_index++];
        double angle = decision_variables[dv_index++];
        
        Optimizer::UAVStrategy uav_strat;
        uav_strat.speed = speed;
        uav_strat.angle = angle;
        
        // 解析第一枚弹药
        double t_d1 = decision_variables[dv_index++];
        double t_f1 = decision_variables[dv_index++];
        uav_strat.grenades.push_back({t_d1, t_f1});
        
        double last_td = t_d1;
        
        // 解析剩余弹药（使用时间间隔）
        for (int i = 1; i < num_grenades; ++i) {
            double delta_t = decision_variables[dv_index++];
            double t_f = decision_variables[dv_index++];
            double current_td = last_td + delta_t;
            uav_strat.grenades.push_back({current_td, t_f});
            last_td = current_td;
        }
        
        strategy[uav_id] = uav_strat;
    }
    
    return strategy;
}

void print_strategy_details(const std::string& missile_id,
                           const Optimizer::StrategyMap& strategy,
                           double obscuration_time,
                           double weight) {
    std::cout << "\n--- 导弹 " << missile_id 
              << " 的最优策略 (遮蔽时间: " << std::fixed << std::setprecision(2) << obscuration_time 
              << "s, 权重: " << weight << ") ---" << std::endl;
    
    for (const auto& [uav_id, uav_strat] : strategy) {
        std::cout << "  UAV: " << uav_id << std::endl;
        std::cout << "    飞行: speed=" << std::setprecision(2) << uav_strat.speed 
                  << ", angle=" << uav_strat.angle << std::endl;
        
        for (size_t i = 0; i < uav_strat.grenades.size(); ++i) {
            const auto& g = uav_strat.grenades[i];
            std::cout << "    弹药 " << (i + 1) 
                      << ": t_deploy=" << g.t_deploy 
                      << "s, t_fuse=" << g.t_fuse << "s" << std::endl;
        }
    }
}

int solve_problem_5() {
    std::cout << "开始求解问题5：多导弹协同遮蔽优化" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    // --- 步骤 0: 威胁评估 ---
    std::cout << "步骤 0: 进行威胁评估..." << std::endl;
    auto threat_weights = ThreatAssessor::assess_threat_weights();
    
    // --- 步骤 1: 高层决策 (基于威胁权重) ---
    std::cout << "\n步骤 1: 执行任务分配..." << std::endl;
    auto assignments = TaskAllocator::assign_tasks_by_threat(threat_weights);
    TaskAllocator::print_assignment_results(assignments, threat_weights);
    
    std::unordered_map<std::string, std::pair<Optimizer::StrategyMap, double>> all_results;
    
    // --- 步骤 2: 低层决策 (优化每个导弹的拦截策略) ---
    std::cout << "\n步骤 2: 开始低层决策优化..." << std::endl;
    
    for (const auto& [missile_id, uav_alloc] : assignments) {
        if (uav_alloc.empty()) {
            std::cout << "跳过导弹 " << missile_id << "（未分配资源）" << std::endl;
            continue;
        }

        std::cout << "\n" << std::string(60, '=') << std::endl;
        auto weight_it = threat_weights.find(missile_id);
        double weight = (weight_it != threat_weights.end()) ? weight_it->second : 0.0;
        
        std::cout << "开始为导弹 " << missile_id 
                  << " (威胁权重: " << std::fixed << std::setprecision(2) << weight 
                  << ") 优化拦截策略..." << std::endl;
        std::cout << "分配的无人机及弹药: ";
        for (const auto& [uav_id, ammo_count] : uav_alloc) {
            std::cout << uav_id << "(" << ammo_count << ") ";
        }
        std::cout << std::endl;
        std::cout << std::string(60, '=') << std::endl;

        // 动态构建优化边界
        std::vector<Optimizer::Bounds> bounds;
        std::vector<std::string> uav_ids_for_task;
        for (const auto& [uav_id, _] : uav_alloc) {
            uav_ids_for_task.push_back(uav_id);
        }
        std::sort(uav_ids_for_task.begin(), uav_ids_for_task.end());
        
        std::cout << "--- 正在计算 t_deploy 的有效边界 ---" << std::endl;
        for (const auto& uav_id : uav_ids_for_task) {
            int num_grenades = uav_alloc.at(uav_id);
            double t_max = BoundaryCalculator::find_max_effective_deploy_time(uav_id, missile_id);
            std::cout << "  " << uav_id << " 的 t_deploy 上边界建议为: " 
                      << std::fixed << std::setprecision(2) << t_max << " s" << std::endl;
            
            // 速度和角度边界
            bounds.emplace_back(Config::UAV_SPEED_MIN, Config::UAV_SPEED_MAX);
            bounds.emplace_back(0.0, 2.0 * M_PI);
            
            // 第一枚弹药的时间边界
            bounds.emplace_back(0.1, t_max);  // t_deploy
            bounds.emplace_back(0.1, 20.0);   // t_fuse
            
            // 剩余弹药的时间间隔边界
            for (int i = 1; i < num_grenades; ++i) {
                bounds.emplace_back(Config::GRENADE_INTERVAL, 10.0);  // delta_t
                bounds.emplace_back(0.1, 20.0);  // t_fuse
            }
        }
        std::cout << "-----------------------------------" << std::endl;
        
        // 创建优化器并求解
        Problem5SubOptimizer optimizer(missile_id, uav_alloc);
        int D = bounds.size();
        std::cout << "该子问题的优化维度为: " << D << std::endl;
        
        Optimizer::DESettings settings;
        settings.population_size = 15 * D;
        settings.max_iterations = 1000;
        settings.tolerance = 0.01;
        settings.verbose = true;
        settings.num_threads = -1;  // 使用所有可用线程
        
        auto start_time = std::chrono::high_resolution_clock::now();
        auto [optimal_strategy, max_time] = optimizer.solve(bounds, settings);
        auto end_time = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        std::cout << "\n对 " << missile_id << " 的优化完成，耗时: " 
                  << duration.count() / 1000.0 << " 秒。" << std::endl;
        std::cout << "最大有效遮蔽时间: " << std::fixed << std::setprecision(4) 
                  << max_time << " s" << std::endl;
        
        all_results[missile_id] = {optimal_strategy, max_time};
    }

    // --- 步骤 3: 汇总、加权评分并保存结果 ---
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "所有优化任务完成，正在生成最终报告..." << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    double total_weighted_score = 0.0;
    
    for (const auto& [missile_id, result_pair] : all_results) {
        auto weight_it = threat_weights.find(missile_id);
        double weight = (weight_it != threat_weights.end()) ? weight_it->second : 0.0;
        double time = result_pair.second;
        total_weighted_score += weight * time;
        
        print_strategy_details(missile_id, result_pair.first, time, weight);
    }
    
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "最终防御策略的加权综合得分: " << std::fixed << std::setprecision(4) 
              << total_weighted_score << std::endl;
    std::cout << std::string(60, '=') << std::endl;

    // 输出详细的最终结果
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "详细策略结果输出" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    for (const auto& [missile_id, result_pair] : all_results) {
        const auto& strategy = result_pair.first;
        double obscuration_time = result_pair.second;
        auto weight_it = threat_weights.find(missile_id);
        double weight = (weight_it != threat_weights.end()) ? weight_it->second : 0.0;
        
        std::cout << "\n" << std::string(40, '-') << std::endl;
        std::cout << "导弹 " << missile_id << " 的完整策略" << std::endl;
        std::cout << "威胁权重: " << std::fixed << std::setprecision(4) << weight << std::endl;
        std::cout << "有效遮蔽时间: " << obscuration_time << " 秒" << std::endl;
        std::cout << std::string(40, '-') << std::endl;
        
        for (const auto& [uav_id, uav_strat] : strategy) {
            std::cout << "\n无人机 " << uav_id << ":" << std::endl;
            std::cout << "  飞行速度: " << std::setprecision(2) << uav_strat.speed << " m/s" << std::endl;
            std::cout << "  飞行角度: " << uav_strat.angle << " rad (" 
                      << (uav_strat.angle * 180.0 / M_PI) << "°)" << std::endl;
            
            // 计算和输出每枚弹药的详细信息
            try {
                CoreObjects::UAV uav(uav_id);
                uav.set_flight_strategy(uav_strat.speed, uav_strat.angle);
                
                for (size_t i = 0; i < uav_strat.grenades.size(); ++i) {
                    const auto& g = uav_strat.grenades[i];
                    Vector3d deploy_pos = uav.get_position(g.t_deploy);
                    auto grenade = uav.deploy_grenade(g.t_deploy, g.t_fuse);
                    Vector3d detonate_pos = grenade->get_detonate_pos();
                    double detonate_time = grenade->get_detonate_time();
                    
                    std::cout << "  弹药 " << (i + 1) << ":" << std::endl;
                    std::cout << "    投放时间: " << std::setprecision(3) << g.t_deploy << " s" << std::endl;
                    std::cout << "    引信时间: " << g.t_fuse << " s" << std::endl;
                    std::cout << "    起爆时间: " << detonate_time << " s" << std::endl;
                    std::cout << "    投放位置: (" << std::setprecision(1) 
                              << deploy_pos[0] << ", " << deploy_pos[1] << ", " << deploy_pos[2] << ")" << std::endl;
                    std::cout << "    起爆位置: (" 
                              << detonate_pos[0] << ", " << detonate_pos[1] << ", " << detonate_pos[2] << ")" << std::endl;
                }
            } catch (const std::exception& e) {
                std::cout << "  计算弹药轨迹时出错: " << e.what() << std::endl;
            }
        }
    }
    
    return 0;
}

} // namespace Problem5

// 主函数
int main() {
    try {
        return Problem5::solve_problem_5();
    } catch (const std::exception& e) {
        std::cerr << "程序执行过程中发生错误: " << e.what() << std::endl;
        return 1;
    }
}
