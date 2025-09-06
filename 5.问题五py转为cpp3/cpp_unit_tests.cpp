#include "cpp_optimizer_wrapper.hpp"
#include <iostream>
#include <cassert>
#include <cmath>
#include <vector>
#include <chrono>

using namespace OptimizerWrapper;
using namespace HighPerformanceDE;

// 简单测试框架
class TestFramework {
private:
    int tests_run_ = 0;
    int tests_passed_ = 0;
    std::string current_test_;
    
public:
    void start_test(const std::string& test_name) {
        current_test_ = test_name;
        tests_run_++;
        std::cout << "🧪 " << test_name << "... ";
    }
    
    void assert_true(bool condition, const std::string& message = "") {
        if (!condition) {
            std::cout << "❌ 失败";
            if (!message.empty()) {
                std::cout << " - " << message;
            }
            std::cout << std::endl;
            throw std::runtime_error("测试断言失败: " + current_test_);
        }
    }
    
    void assert_near(double actual, double expected, double tolerance = 1e-6, const std::string& message = "") {
        if (std::abs(actual - expected) > tolerance) {
            std::cout << "❌ 失败 - 期望: " << expected << ", 实际: " << actual;
            if (!message.empty()) {
                std::cout << " - " << message;
            }
            std::cout << std::endl;
            throw std::runtime_error("数值断言失败: " + current_test_);
        }
    }
    
    void pass() {
        tests_passed_++;
        std::cout << "✅ 通过" << std::endl;
    }
    
    void print_summary() {
        std::cout << "\n" << std::string(50, '=') << std::endl;
        std::cout << "测试总结: " << tests_passed_ << "/" << tests_run_ << " 通过";
        if (tests_passed_ == tests_run_) {
            std::cout << " 🎉";
        }
        std::cout << std::endl;
        std::cout << std::string(50, '=') << std::endl;
    }
    
    bool all_passed() const {
        return tests_passed_ == tests_run_;
    }
};

TestFramework test_framework;

// 简单的二次函数用于测试
double quadratic_function(const Vector& x) {
    double result = 0.0;
    for (int i = 0; i < x.size(); ++i) {
        result += (x[i] - i) * (x[i] - i);  // 最优解在 (0, 1, 2, ..., n-1)
    }
    return result;
}

// 带约束的函数
double constrained_function(const Vector& x) {
    double result = x.squaredNorm();
    
    // 软约束：惩罚离原点太远的解
    for (int i = 0; i < x.size(); ++i) {
        if (std::abs(x[i]) > 5.0) {
            result += 1000.0 * (std::abs(x[i]) - 5.0);
        }
    }
    
    return result;
}

void test_adaptive_parameter_manager() {
    test_framework.start_test("AdaptiveParameterManager基础功能");
    
    AdaptiveParameterManager manager(10, 42);
    
    // 测试初始参数生成
    auto [F1, CR1] = manager.generate_parameters();
    test_framework.assert_true(F1 >= 0.0 && F1 <= 2.0, "F参数范围检查");
    test_framework.assert_true(CR1 >= 0.0 && CR1 <= 1.0, "CR参数范围检查");
    
    // 测试成功参数添加和更新
    manager.add_success(0.8, 0.7, MutationStrategy::RAND_1);
    manager.add_success(0.6, 0.5, MutationStrategy::BEST_1);
    manager.update_parameters();
    
    auto [F2, CR2] = manager.get_current_means();
    test_framework.assert_true(F2 > 0.0, "参数更新后F有效");
    test_framework.assert_true(CR2 > 0.0, "参数更新后CR有效");
    
    test_framework.pass();
}

void test_boundary_processor() {
    test_framework.start_test("BoundaryProcessor边界处理");
    
    Vector lower(3);
    lower << -2.0, -1.0, 0.0;
    Vector upper(3); 
    upper << 2.0, 1.0, 5.0;
    
    BoundaryProcessor processor(lower, upper, BoundaryHandling::CLIP, 42);
    
    // 测试超出边界的向量
    Vector individual(3);
    individual << -5.0, 2.0, 10.0;  // 所有维度都超出边界
    
    processor.process(individual);
    
    test_framework.assert_near(individual[0], -2.0, 1e-10, "下边界截断");
    test_framework.assert_near(individual[1], 1.0, 1e-10, "上边界截断");
    test_framework.assert_near(individual[2], 5.0, 1e-10, "上边界截断");
    
    test_framework.pass();
}

void test_solution_cache() {
    test_framework.start_test("SolutionCache解缓存");
    
    SolutionCache cache(100, 1e-10);
    
    Vector solution1(3);
    solution1 << 1.0, 2.0, 3.0;
    
    Vector solution2(3); 
    solution2 << 1.0, 2.0, 3.0 + 1e-12;  // 极小差异，应该被认为相同
    
    // 存储解
    cache.store(solution1, 42.0);
    
    // 查找相同解
    double fitness;
    bool found = cache.lookup(solution2, fitness);
    
    test_framework.assert_true(found, "应该找到相似解");
    test_framework.assert_near(fitness, 42.0, 1e-10, "缓存的适应度值");
    
    auto [hits, misses] = cache.get_statistics();
    test_framework.assert_true(hits == 1, "缓存命中统计");
    
    test_framework.pass();
}

void test_simple_optimization() {
    test_framework.start_test("简单优化问题求解");
    
    // 2维二次函数优化
    std::vector<std::pair<double, double>> bounds = {{-5.0, 5.0}, {-5.0, 5.0}};
    
    AdaptiveDESettings settings;
    settings.population_size = 40;
    settings.max_iterations = 200;
    settings.tolerance = 1e-6;
    settings.verbose = false;
    settings.random_seed = 42;
    
    auto result = adaptive_differential_evolution(quadratic_function, bounds, settings);
    
    test_framework.assert_true(result.converged, "优化应该收敛");
    test_framework.assert_near(result.best_fitness, 0.0, 1e-4, "应该找到全局最优");
    test_framework.assert_near(result.best_solution[0], 0.0, 0.1, "第一维最优值");
    test_framework.assert_near(result.best_solution[1], 1.0, 0.1, "第二维最优值");
    
    test_framework.pass();
}

void test_constrained_optimization() {
    test_framework.start_test("约束优化问题");
    
    std::vector<std::pair<double, double>> bounds = {{-10.0, 10.0}, {-10.0, 10.0}, {-10.0, 10.0}};
    
    AdaptiveDESettings settings;
    settings.population_size = 60;
    settings.max_iterations = 300;
    settings.tolerance = 1e-5;
    settings.verbose = false;
    settings.boundary_handling = BoundaryHandling::REFLECT;
    settings.random_seed = 42;
    
    auto result = adaptive_differential_evolution(constrained_function, bounds, settings);
    
    test_framework.assert_true(std::isfinite(result.best_fitness), "适应度应该是有限值");
    test_framework.assert_true(result.best_fitness >= 0.0, "适应度应该非负");
    
    // 检查解是否在合理范围内
    for (double x : result.best_solution) {
        test_framework.assert_true(std::abs(x) <= 6.0, "解应该在合理范围内");
    }
    
    test_framework.pass();
}

void test_problem5_optimizer() {
    test_framework.start_test("Problem5CppOptimizer集成测试");
    
    // 创建简单的UAV分配
    std::unordered_map<std::string, int> uav_assignments = {
        {"FY1", 1},  // 每个UAV一枚弹药，简化测试
        {"FY2", 1}
    };
    
    std::string missile_id = "M1";
    
    // 构建边界
    std::vector<std::pair<double, double>> bounds;
    for (int uav = 0; uav < 2; ++uav) {
        bounds.emplace_back(70.0, 140.0);    // 速度
        bounds.emplace_back(0.0, 2 * M_PI);  // 角度
        bounds.emplace_back(0.1, 20.0);      // t_deploy
        bounds.emplace_back(0.1, 10.0);      // t_fuse
    }
    
    auto optimizer = Problem5CppOptimizer::create(missile_id, uav_assignments, bounds);
    
    SimpleSettings settings;
    settings.population_size = 40;
    settings.max_iterations = 100;  // 减少迭代次数加快测试
    settings.verbose = false;
    settings.random_seed = 42;
    
    auto result = optimizer->optimize(settings);
    
    test_framework.assert_true(std::isfinite(result.best_fitness), "应该产生有限适应度");
    test_framework.assert_true(result.best_solution.size() == bounds.size(), "解向量维度正确");
    test_framework.assert_true(result.execution_time > 0.0, "执行时间应该为正");
    test_framework.assert_true(result.total_evaluations > 0, "应该有函数评估");
    
    test_framework.pass();
}

void test_settings_validation() {
    test_framework.start_test("设置验证功能");
    
    // 测试有效设置
    SimpleSettings valid_settings;
    valid_settings.population_size = 50;
    valid_settings.max_iterations = 100;
    valid_settings.tolerance = 1e-6;
    valid_settings.boundary_handling = "reflect";
    
    test_framework.assert_true(Utils::validate_settings(valid_settings), "有效设置应该通过验证");
    
    // 测试无效设置
    SimpleSettings invalid_settings = valid_settings;
    invalid_settings.max_iterations = -1;
    
    test_framework.assert_true(!Utils::validate_settings(invalid_settings), "无效设置应该被拒绝");
    
    // 测试边界验证
    std::vector<std::pair<double, double>> valid_bounds = {{-1.0, 1.0}, {0.0, 5.0}};
    test_framework.assert_true(Utils::validate_bounds(valid_bounds), "有效边界");
    
    std::vector<std::pair<double, double>> invalid_bounds = {{1.0, -1.0}};  // 下界大于上界
    test_framework.assert_true(!Utils::validate_bounds(invalid_bounds), "无效边界应该被拒绝");
    
    test_framework.pass();
}

void test_performance_characteristics() {
    test_framework.start_test("性能特征测试");
    
    // 测试小规模问题的快速求解
    std::vector<std::pair<double, double>> small_bounds = {{-2.0, 2.0}, {-2.0, 2.0}};
    
    AdaptiveDESettings settings;
    settings.population_size = 20;
    settings.max_iterations = 50;
    settings.tolerance = 1e-4;
    settings.verbose = false;
    settings.random_seed = 42;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    auto result = adaptive_differential_evolution(
        [](const Vector& x) { return x.squaredNorm(); },
        small_bounds, 
        settings
    );
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    test_framework.assert_true(duration.count() < 5000, "小规模问题应该快速求解(<5s)");
    test_framework.assert_true(result.best_fitness < 1e-2, "应该找到较好的解");
    test_framework.assert_true(result.performance_stats.total_evaluations < 2000, "函数评估次数合理");
    
    test_framework.pass();
}

void test_memory_safety() {
    test_framework.start_test("内存安全测试");
    
    // 测试多次创建和销毁优化器
    for (int i = 0; i < 10; ++i) {
        std::unordered_map<std::string, int> uav_assignments = {{"FY1", 1}};
        std::vector<std::pair<double, double>> bounds = {{70.0, 140.0}, {0.0, 6.28}, {0.1, 10.0}, {0.1, 5.0}};
        
        auto optimizer = Problem5CppOptimizer::create("M1", uav_assignments, bounds);
        
        SimpleSettings settings;
        settings.population_size = 20;
        settings.max_iterations = 10;
        settings.verbose = false;
        
        auto result = optimizer->optimize(settings);
        
        // 基本检查确保没有内存错误
        test_framework.assert_true(std::isfinite(result.best_fitness), "迭代" + std::to_string(i) + "产生有效结果");
    }
    
    test_framework.pass();
}

int main() {
    try {
        std::cout << "🧪 高性能C++自适应差分进化算法单元测试" << std::endl;
        std::cout << std::string(60, '=') << std::endl;
        
        // 执行所有测试
        test_adaptive_parameter_manager();
        test_boundary_processor();
        test_solution_cache();
        test_simple_optimization();
        test_constrained_optimization();
        test_problem5_optimizer();
        test_settings_validation();
        test_performance_characteristics();
        test_memory_safety();
        
        // 打印测试总结
        test_framework.print_summary();
        
        return test_framework.all_passed() ? 0 : 1;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ 测试过程中发生异常: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "❌ 发生未知异常" << std::endl;
        return 1;
    }
}
