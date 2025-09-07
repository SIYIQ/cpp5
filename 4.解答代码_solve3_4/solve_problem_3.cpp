#include "solve_problem_3.hpp"
#include "config.hpp"
#define _USE_MATH_DEFINES
#include <iostream>
#include <iomanip>
#include <chrono>
#include <cmath>
#include <stdexcept>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Problem3Optimizer 实现
Problem3Optimizer::Problem3Optimizer() 
    : ObscurationOptimizer("M1", {{"FY1", 3}}) {
}

StrategyMap Problem3Optimizer::parse_decision_variables(const std::vector<double>& dv) {
    if (dv.size() != 8) {
        throw std::invalid_argument("Decision variables must have 8 elements");
    }
    
    double speed = dv[0];
    double angle = dv[1];
    double t_d1 = dv[2];
    double t_f1 = dv[3];
    double delta_t2 = dv[4];
    double t_f2 = dv[5];
    double delta_t3 = dv[6];
    double t_f3 = dv[7];
    
    // 从时间增量计算绝对投放时间
    double t_d2 = t_d1 + delta_t2;
    double t_d3 = t_d2 + delta_t3;
    
    StrategyMap strategy = {
        {"FY1", {
            speed,
            angle,
            {
                {t_d1, t_f1},
                {t_d2, t_f2},
                {t_d3, t_f3}
            }
        }}
    };
    
    return strategy;
}

void solve_problem_3() {
    // 1. 定义决策变量的边界
    Bounds bounds = {
        {UAV_SPEED_MIN, UAV_SPEED_MAX},    // speed
        {0, 2 * M_PI},                     // angle
        {0.1, 25.0},                       // t_deploy1
        {0.1, 20.0},                       // t_fuse1
        {GRENADE_INTERVAL, 10.0},          // delta_t2 (min 1.0s)
        {0.1, 20.0},                       // t_fuse2
        {GRENADE_INTERVAL, 10.0},          // delta_t3 (min 1.0s)
        {0.1, 20.0}                        // t_fuse3
    };

    // 2. 实例化优化器
    Problem3Optimizer optimizer;
    
    // 3. 设置求解器参数
    DESettings solver_options;
    solver_options.popsize = 150;      // 增加种群规模
    solver_options.maxiter = 1000;     // 增加最大迭代次数
    solver_options.tol = 0.01;
    solver_options.disp = true;
    solver_options.workers = 1;        // 暂时使用单线程
    
    std::cout << std::string(50, '=') << std::endl;
    std::cout << "开始求解问题三：FY1 vs M1 (3枚干扰弹)" << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    auto [optimal_strategy_dict, max_time] = optimizer.solve(bounds, solver_options);
    auto end_time = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    std::cout << "\n优化完成，耗时: " << duration.count() / 1000.0 << " 秒。" << std::endl;

    // 4. 打印结果
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "问题三 最优策略详情" << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    std::cout << "最大有效遮蔽时间: " << std::fixed << std::setprecision(4) << max_time << " s" << std::endl;
    
    // 格式化打印策略
    const auto& uav_strat = optimal_strategy_dict.at("FY1");
    std::cout << "  无人机飞行速度: " << std::fixed << std::setprecision(4) << uav_strat.speed << " m/s" << std::endl;
    std::cout << "  无人机飞行角度: " << std::fixed << std::setprecision(4) << uav_strat.angle << " rad" << std::endl;
    
    for (size_t i = 0; i < uav_strat.grenades.size(); ++i) {
        const auto& g = uav_strat.grenades[i];
        std::cout << "  - 干扰弹 " << (i + 1) << ": 投放时间=" 
                  << std::fixed << std::setprecision(4) << g.t_deploy 
                  << "s, 引信时间=" << std::fixed << std::setprecision(4) << g.t_fuse << "s" << std::endl;
    }

    std::cout << std::string(50, '=') << std::endl;
}

int main() {
    try {
        solve_problem_3();
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
