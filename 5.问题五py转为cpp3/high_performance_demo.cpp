#include "cpp_optimizer_wrapper.hpp"
#include <iostream>
#include <chrono>
#include <vector>
#include <unordered_map>

using namespace OptimizerWrapper;

// 创建测试用例
std::pair<std::unordered_map<std::string, int>, std::vector<std::pair<double, double>>> 
create_test_case() {
    // UAV分配：每个UAV分配2枚弹药
    std::unordered_map<std::string, int> uav_assignments = {
        {"FY1", 2},
        {"FY2", 2},
        {"FY3", 2}
    };
    
    // 构建优化边界
    std::vector<std::pair<double, double>> bounds;
    
    // 飞行参数范围
    const double UAV_SPEED_MIN = 70.0;
    const double UAV_SPEED_MAX = 140.0;
    const double GRENADE_INTERVAL = 1.0;
    
    for (const auto& [uav_id, num_grenades] : uav_assignments) {
        // 飞行速度和角度
        bounds.emplace_back(UAV_SPEED_MIN, UAV_SPEED_MAX);  // 速度
        bounds.emplace_back(0.0, 2.0 * M_PI);              // 角度
        
        // 第一枚弹药：投放时间和引信时间
        bounds.emplace_back(0.1, 30.0);  // t_deploy
        bounds.emplace_back(0.1, 20.0);  // t_fuse
        
        // 剩余弹药：时间间隔和引信时间
        for (int i = 1; i < num_grenades; ++i) {
            bounds.emplace_back(GRENADE_INTERVAL, 10.0);  // delta_t
            bounds.emplace_back(0.1, 20.0);              // t_fuse
        }
    }
    
    return {uav_assignments, bounds};
}

void demo_basic_optimization() {
    std::cout << "🚀 高性能C++自适应差分进化算法演示\n";
    std::cout << std::string(60, '=') << "\n";
    
    // 创建测试场景
    auto [uav_assignments, bounds] = create_test_case();
    std::string missile_id = "M1";
    
    std::cout << "测试场景:\n";
    std::cout << "  导弹: " << missile_id << "\n";
    std::cout << "  UAV分配: ";
    for (const auto& [uav_id, count] : uav_assignments) {
        std::cout << uav_id << "(" << count << ") ";
    }
    std::cout << "\n";
    std::cout << "  优化维度: " << bounds.size() << "\n\n";
    
    // 创建优化器
    auto optimizer = Problem5CppOptimizer::create(missile_id, uav_assignments, bounds);
    
    // 获取推荐设置
    auto settings = optimizer->get_recommended_settings();
    settings.max_iterations = 300;  // 演示用，减少迭代次数
    settings.verbose = true;
    
    std::cout << "算法设置:\n";
    std::cout << "  种群大小: " << settings.population_size << "\n";
    std::cout << "  最大迭代: " << settings.max_iterations << "\n";
    std::cout << "  边界处理: " << settings.boundary_handling << "\n";
    std::cout << "  并行线程: " << (settings.num_threads == -1 ? "自动" : std::to_string(settings.num_threads)) << "\n\n";
    
    // 执行优化
    std::cout << "开始优化...\n";
    auto start_time = std::chrono::high_resolution_clock::now();
    
    auto result = optimizer->optimize(settings);
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    // 显示结果
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "🎯 优化结果\n";
    std::cout << std::string(60, '=') << "\n";
    
    std::cout << "最优遮蔽时间: " << std::fixed << std::setprecision(4) 
              << (-result.best_fitness) << " 秒\n";
    std::cout << "算法迭代次数: " << result.iterations << "\n";
    std::cout << "函数评估次数: " << result.total_evaluations << "\n";
    std::cout << "总执行时间: " << total_duration.count() << " ms\n";
    std::cout << "收敛状态: " << (result.converged ? "✅ 成功" : "❌ 未收敛") << "\n";
    std::cout << "缓存命中率: " << std::setprecision(1) << (result.cache_hit_rate * 100) << "%\n";
    
    // 显示最优策略的前几个参数
    std::cout << "\n最优策略参数 (前10个):\n";
    for (size_t i = 0; i < std::min(size_t(10), result.best_solution.size()); ++i) {
        std::cout << "  x[" << i << "] = " << std::setprecision(4) << result.best_solution[i] << "\n";
    }
    if (result.best_solution.size() > 10) {
        std::cout << "  ... (共 " << result.best_solution.size() << " 个参数)\n";
    }
    
    // 保存收敛历史
    Utils::save_convergence_history(result.convergence_history, "cpp_convergence_history.csv");
}

void demo_performance_comparison() {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "⚡ 性能对比测试\n";
    std::cout << std::string(60, '=') << "\n";
    
    // 创建测试场景
    auto [uav_assignments, bounds] = create_test_case();
    std::string missile_id = "M1";
    
    // 执行性能对比
    int num_runs = 3; // 演示用，减少运行次数
    auto comparison_results = Benchmark::compare_algorithms(
        missile_id, uav_assignments, bounds, num_runs
    );
    
    // 生成报告
    Benchmark::generate_performance_report(comparison_results, "cpp_performance_report.html");
    
    // 显示简要结果
    std::cout << "\n📊 性能对比结果:\n";
    for (const auto& result : comparison_results) {
        std::cout << "\n算法: " << result.algorithm_name << "\n";
        std::cout << "  平均时间: " << std::fixed << std::setprecision(2) << result.avg_time << " 秒\n";
        std::cout << "  平均适应度: " << std::scientific << std::setprecision(4) << result.avg_fitness << "\n";
        std::cout << "  最佳适应度: " << result.best_fitness << "\n";
        std::cout << "  成功率: " << std::setprecision(1) << (result.success_rate * 100) << "%\n";
    }
    
    std::cout << "\n详细报告已保存到: cpp_performance_report.html\n";
}

void demo_system_info() {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "💻 系统信息\n";
    std::cout << std::string(60, '=') << "\n";
    
    Utils::print_system_info();
    
    std::cout << "\n推荐线程数: " << Utils::get_recommended_thread_count() << "\n";
}

void demo_parameter_tuning() {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "🔧 参数调优演示\n";
    std::cout << std::string(60, '=') << "\n";
    
    auto [uav_assignments, bounds] = create_test_case();
    std::string missile_id = "M1";
    
    // 测试不同的设置
    std::vector<SimpleSettings> settings_variants = {
        // 小种群，快速测试
        {60, 200, 0.01, true, -1, true, true, -1, "reflect"},
        // 大种群，高精度
        {120, 400, 0.005, true, -1, true, true, -1, "reflect"},
        // 中等设置，平衡性能
        {90, 300, 0.01, true, -1, true, true, -1, "reflect"}
    };
    
    std::vector<std::string> variant_names = {
        "快速模式", "高精度模式", "平衡模式"
    };
    
    for (size_t i = 0; i < settings_variants.size(); ++i) {
        std::cout << "\n测试 " << variant_names[i] << ":\n";
        
        auto optimizer = Problem5CppOptimizer::create(missile_id, uav_assignments, bounds);
        auto& settings = settings_variants[i];
        settings.verbose = false; // 减少输出
        
        auto start_time = std::chrono::high_resolution_clock::now();
        auto result = optimizer->optimize(settings);
        auto end_time = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        std::cout << "  遮蔽时间: " << std::fixed << std::setprecision(4) << (-result.best_fitness) << " 秒\n";
        std::cout << "  执行时间: " << duration.count() << " ms\n";
        std::cout << "  函数评估: " << result.total_evaluations << " 次\n";
        std::cout << "  收敛状态: " << (result.converged ? "成功" : "未收敛") << "\n";
    }
    
    std::cout << "\n💡 调优建议:\n";
    std::cout << "  - 快速模式适合初步测试和参数探索\n";
    std::cout << "  - 高精度模式适合最终优化和生产使用\n"; 
    std::cout << "  - 平衡模式在大多数情况下提供良好的性价比\n";
}

int main() {
    try {
        std::cout << "🎯 高性能C++自适应差分进化算法完整演示\n";
        std::cout << "版本: 1.0.0\n";
        std::cout << "构建时间: " << __DATE__ << " " << __TIME__ << "\n\n";
        
        // 显示系统信息
        demo_system_info();
        
        // 基础优化演示
        demo_basic_optimization();
        
        // 参数调优演示
        demo_parameter_tuning();
        
        // 性能对比测试
        demo_performance_comparison();
        
        std::cout << "\n" << std::string(60, '=') << "\n";
        std::cout << "🎉 演示完成！\n";
        std::cout << "生成的文件:\n";
        std::cout << "  - cpp_convergence_history.csv: 收敛历史\n";
        std::cout << "  - cpp_performance_report.html: 性能报告\n";
        std::cout << std::string(60, '=') << "\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ 演示过程中发生错误: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "❌ 发生未知错误" << std::endl;
        return 1;
    }
}
