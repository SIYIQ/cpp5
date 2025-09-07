#include "solve_problem_4.hpp"
#include "boundary_calculator.hpp"
#include "config.hpp"
#define _USE_MATH_DEFINES
#include <iostream>
#include <iomanip>
#include <chrono>
#include <cmath>
#include <unordered_map>
#include <vector>
#include <string>
#include <stdexcept>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Problem4Optimizer 实现
Problem4Optimizer::Problem4Optimizer() 
    : ObscurationOptimizer("M1", {{"FY1", 1}, {"FY2", 1}, {"FY3", 1}}) {
}

StrategyMap Problem4Optimizer::parse_decision_variables(const std::vector<double>& dv) {
    if (dv.size() != 12) {
        throw std::invalid_argument("Decision variables must have 12 elements");
    }
    
    // dv = [s1, a1, td1, tf1, s2, a2, td2, tf2, s3, a3, td3, tf3]
    double s1 = dv[0], a1 = dv[1], td1 = dv[2], tf1 = dv[3];
    double s2 = dv[4], a2 = dv[5], td2 = dv[6], tf2 = dv[7];
    double s3 = dv[8], a3 = dv[9], td3 = dv[10], tf3 = dv[11];
    
    StrategyMap strategy = {
        {"FY1", {s1, a1, {{td1, tf1}}}},
        {"FY2", {s2, a2, {{td2, tf2}}}},
        {"FY3", {s3, a3, {{td3, tf3}}}}
    };
    
    return strategy;
}

void solve_problem_4() {
    const std::vector<std::string> UAV_IDS = {"FY1", "FY2", "FY3"};
    const std::string MISSILE_ID = "M1";

    // 1. 为每架无人机动态计算 t_deploy 的上边界
    std::cout << "--- 正在为各无人机计算 t_deploy 的有效边界 ---" << std::endl;
    std::unordered_map<std::string, double> t_deploy_max_bounds;
    
    for (const auto& uav_id : UAV_IDS) {
        // 注意: boundary_calculator 内部的最优飞行角度假设可能需要调整
        // 对于FY2, FY3，最优角度不一定是 pi，但 pi 作为一个保守的"向前冲"策略来定界是合理的。
        double t_max = find_max_effective_deploy_time(uav_id, MISSILE_ID);
        t_deploy_max_bounds[uav_id] = t_max;
        std::cout << "  " << uav_id << " 的 t_deploy 上边界建议为: " 
                  << std::fixed << std::setprecision(2) << t_max << " s" << std::endl;
    }
    std::cout << std::string(49, '-') << std::endl;

    // 2. 定义12个决策变量的总边界
    Bounds bounds_fy1 = {
        {UAV_SPEED_MIN, UAV_SPEED_MAX}, {0, 2 * M_PI},
        {0.1, t_deploy_max_bounds["FY1"]}, {0.1, 20.0}
    };
    Bounds bounds_fy2 = {
        {UAV_SPEED_MIN, UAV_SPEED_MAX}, {0, 2 * M_PI},
        {0.1, t_deploy_max_bounds["FY2"]}, {0.1, 20.0}
    };
    Bounds bounds_fy3 = {
        {UAV_SPEED_MIN, UAV_SPEED_MAX}, {0, 2 * M_PI},
        {0.1, t_deploy_max_bounds["FY3"]}, {0.1, 20.0}
    };
    
    Bounds bounds;
    bounds.insert(bounds.end(), bounds_fy1.begin(), bounds_fy1.end());
    bounds.insert(bounds.end(), bounds_fy2.begin(), bounds_fy2.end());
    bounds.insert(bounds.end(), bounds_fy3.begin(), bounds_fy3.end());

    // 3. 实例化优化器
    Problem4Optimizer optimizer;
    
    // 4. 设置求解器参数
    const int D = 12; // 维度
    DESettings solver_options;
    solver_options.popsize = 15 * D;   // 经验法则: popsize 至少为 10*D
    solver_options.maxiter = 2000;     // 增加迭代次数
    solver_options.tol = 0.01;
    solver_options.disp = true;
    solver_options.workers = 1;        // 暂时使用单线程
    
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "开始求解问题四：FY1, FY2, FY3 vs M1 (各1枚)" << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    auto [optimal_strategy_dict, max_time] = optimizer.solve(bounds, solver_options);
    auto end_time = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    std::cout << "\n优化完成，耗时: " << duration.count() / 1000.0 << " 秒。" << std::endl;

    // 5. 打印结果
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "问题四 最优策略详情" << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    std::cout << "最大总有效遮蔽时间: " << std::fixed << std::setprecision(4) << max_time << " s" << std::endl;
    
    for (const auto& [uav_id, uav_strat] : optimal_strategy_dict) {
        std::cout << "\n--- " << uav_id << " 策略 ---" << std::endl;
        std::cout << "  飞行速度: " << std::fixed << std::setprecision(4) << uav_strat.speed << " m/s" << std::endl;
        std::cout << "  飞行角度: " << std::fixed << std::setprecision(4) << uav_strat.angle << " rad" << std::endl;
        
        const auto& g = uav_strat.grenades[0];
        std::cout << "  投放时间: " << std::fixed << std::setprecision(4) << g.t_deploy 
                  << "s, 引信时间: " << std::fixed << std::setprecision(4) << g.t_fuse << "s" << std::endl;
    }

    std::cout << std::string(50, '=') << std::endl;
}

int main() {
    try {
        solve_problem_4();
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
