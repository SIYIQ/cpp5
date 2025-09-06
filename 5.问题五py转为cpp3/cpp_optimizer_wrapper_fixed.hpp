#pragma once

// 修复版本的优化器包装器头文件
// 主要修复: 线程安全、异常安全、参数验证

#include "high_performance_adaptive_de.hpp"
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <atomic>       // ✅ 添加: 线程安全支持
#include <mutex>        // ✅ 添加: 互斥锁支持
#include <stdexcept>    // ✅ 添加: 异常处理支持

namespace OptimizerWrapper {

// 前向声明
class Problem5Objective;

// 优化结果的简化版本，便于与Python交互
struct SimpleOptimizationResult {
    std::vector<double> best_solution;
    double best_fitness = std::numeric_limits<double>::infinity();  // ✅ 修复: 默认值
    int iterations = 0;                                            // ✅ 修复: 默认值
    double execution_time = 0.0;                                   // ✅ 修复: 默认值
    bool converged = false;                                        // ✅ 修复: 默认值
    std::vector<double> convergence_history;
    
    // 性能统计
    size_t total_evaluations = 0;                                  // ✅ 修复: 默认值
    double cache_hit_rate = 0.0;                                   // ✅ 修复: 默认值
    std::vector<double> strategy_success_rates;
    std::vector<double> final_parameters; // [mean_F, mean_CR]
    
    // ✅ 添加: 错误信息
    std::string error_message;
    bool has_error = false;
};

// 算法设置的简化版本
struct SimpleSettings {
    int population_size = 0;           // 0表示自动计算
    int max_iterations = 1000;
    double tolerance = 1e-6;
    bool verbose = true;
    int num_threads = -1;              // -1表示使用所有线程
    bool enable_caching = true;
    bool adaptive_population = true;
    int random_seed = -1;              // -1表示随机种子
    std::string boundary_handling = "reflect"; // "clip", "reflect", "reinitialize", "midpoint"
    
    // ✅ 添加: 参数验证方法
    bool validate() const {
        if (max_iterations <= 0) return false;
        if (tolerance <= 0.0) return false;
        if (num_threads < -1) return false;
        
        const std::vector<std::string> valid_boundaries = {
            "clip", "reflect", "reinitialize", "midpoint"
        };
        return std::find(valid_boundaries.begin(), valid_boundaries.end(), 
                        boundary_handling) != valid_boundaries.end();
    }
    
    // 转换为内部设置
    HighPerformanceDE::AdaptiveDESettings to_internal_settings() const;
};

// 问题5专用的C++优化器包装类 (线程安全版本)
class Problem5CppOptimizer {
private:
    std::string missile_id_;
    std::unordered_map<std::string, int> uav_assignments_;
    std::unique_ptr<Problem5Objective> objective_;
    std::vector<std::pair<double, double>> bounds_;
    int dimension_;
    
    // ✅ 修复: 线程安全的性能统计
    mutable std::mutex stats_mutex_;                               // 互斥锁保护统计数据
    mutable std::atomic<double> last_optimization_time_{0.0};      // 原子操作
    mutable std::atomic<size_t> last_total_evaluations_{0};        // 原子操作
    mutable std::atomic<bool> is_optimizing_{false};               // 优化状态标志
    
    // ✅ 添加: 创建失败结果的辅助方法
    SimpleOptimizationResult create_failed_result(const std::string& error_msg) const;
    
public:
    // 构造函数
    explicit Problem5CppOptimizer(
        const std::string& missile_id,
        const std::unordered_map<std::string, int>& uav_assignments
    );
    
    ~Problem5CppOptimizer();
    
    // ✅ 修复: 删除拷贝构造和赋值，确保线程安全
    Problem5CppOptimizer(const Problem5CppOptimizer&) = delete;
    Problem5CppOptimizer& operator=(const Problem5CppOptimizer&) = delete;
    Problem5CppOptimizer(Problem5CppOptimizer&&) = default;
    Problem5CppOptimizer& operator=(Problem5CppOptimizer&&) = default;
    
    // 设置优化边界 (带验证)
    bool set_bounds(const std::vector<std::pair<double, double>>& bounds);
    
    // ✅ 修复: 异常安全的主要优化接口
    SimpleOptimizationResult optimize(const SimpleSettings& settings = SimpleSettings()) noexcept;
    
    // 性能测试接口
    SimpleOptimizationResult benchmark_against_scipy(
        const SimpleSettings& settings,
        int num_runs = 3
    ) noexcept;
    
    // 获取推荐设置
    SimpleSettings get_recommended_settings() const;
    
    // ✅ 修复: 线程安全的工具函数
    int get_dimension() const { return dimension_; }
    double get_last_optimization_time() const { 
        return last_optimization_time_.load(std::memory_order_relaxed); 
    }
    size_t get_last_evaluations() const { 
        return last_total_evaluations_.load(std::memory_order_relaxed); 
    }
    bool is_optimizing() const { 
        return is_optimizing_.load(std::memory_order_relaxed); 
    }
    
    // ✅ 修复: 异常安全的静态工厂方法
    static std::unique_ptr<Problem5CppOptimizer> create(
        const std::string& missile_id,
        const std::unordered_map<std::string, int>& uav_assignments,
        const std::vector<std::pair<double, double>>& bounds
    ) noexcept;
};

// 目标函数包装类（连接到原有的Python逻辑）
class Problem5Objective {
private:
    std::string missile_id_;
    std::unordered_map<std::string, int> uav_assignments_;
    
    // 决策变量解析
    struct UAVStrategy {
        double speed = 100.0;   // ✅ 修复: 默认值
        double angle = 0.0;     // ✅ 修复: 默认值
        std::vector<std::pair<double, double>> grenades; // {t_deploy, t_fuse}
        
        // ✅ 添加: 验证方法
        bool validate() const {
            if (speed < 70.0 || speed > 140.0) return false;
            if (angle < 0.0 || angle > 2.0 * M_PI) return false;
            for (const auto& [t_deploy, t_fuse] : grenades) {
                if (t_deploy < 0.1 || t_fuse < 0.1 || t_fuse > 20.0) return false;
            }
            return true;
        }
    };
    
    // ✅ 修复: 异常安全的解析方法
    std::unordered_map<std::string, UAVStrategy> parse_decision_variables(
        const std::vector<double>& decision_variables
    ) const noexcept;
    
    // ✅ 修复: 异常安全的核心计算
    double calculate_obscuration_time(
        const std::unordered_map<std::string, UAVStrategy>& strategies
    ) const noexcept;
    
    // ✅ 修复: 异常安全的约束检查
    double calculate_constraint_violation(
        const std::vector<double>& decision_variables
    ) const noexcept;
    
public:
    Problem5Objective(
        const std::string& missile_id,
        const std::unordered_map<std::string, int>& uav_assignments
    );
    
    ~Problem5Objective();
    
    // ✅ 修复: 异常安全的主要目标函数接口
    double operator()(const HighPerformanceDE::Vector& x) const noexcept;
    
    // 获取统计信息
    struct Statistics {
        std::atomic<size_t> total_calls{0};
        std::atomic<size_t> constraint_violations{0};
        std::atomic<double> avg_evaluation_time{0.0};
        std::atomic<double> best_fitness_seen{std::numeric_limits<double>::infinity()};
        
        // ✅ 添加: 线程安全的重置方法
        void reset() {
            total_calls.store(0);
            constraint_violations.store(0);
            avg_evaluation_time.store(0.0);
            best_fitness_seen.store(std::numeric_limits<double>::infinity());
        }
    };
    
    mutable Statistics stats_;
    const Statistics& get_statistics() const { return stats_; }
    void reset_statistics() { stats_.reset(); }
};

// 性能基准测试工具
namespace Benchmark {
    
    struct ComparisonResult {
        std::string algorithm_name;
        double avg_time = 0.0;              // ✅ 修复: 默认值
        double avg_fitness = 0.0;           // ✅ 修复: 默认值
        double std_fitness = 0.0;           // ✅ 修复: 默认值
        double best_fitness = 0.0;          // ✅ 修复: 默认值
        int avg_iterations = 0;             // ✅ 修复: 默认值
        double success_rate = 0.0;          // ✅ 修复: 默认值
        
        // 相对于基准的改进
        double time_improvement_percent = 0.0;
        double fitness_improvement_percent = 0.0;
        
        // ✅ 添加: 验证方法
        bool is_valid() const {
            return avg_time > 0.0 && std::isfinite(avg_fitness) && 
                   success_rate >= 0.0 && success_rate <= 1.0;
        }
    };
    
    // ✅ 修复: 异常安全的对比函数
    std::vector<ComparisonResult> compare_algorithms(
        const std::string& missile_id,
        const std::unordered_map<std::string, int>& uav_assignments,
        const std::vector<std::pair<double, double>>& bounds,
        int num_runs = 5
    ) noexcept;
    
    // ✅ 修复: 异常安全的报告生成
    bool generate_performance_report(
        const std::vector<ComparisonResult>& results,
        const std::string& output_file = "cpp_performance_report.html"
    ) noexcept;
}

// 便利函数
namespace Utils {
    
    // Python兼容性工具
    std::vector<double> eigen_to_vector(const HighPerformanceDE::Vector& v);
    HighPerformanceDE::Vector vector_to_eigen(const std::vector<double>& v);
    
    // ✅ 修复: 异常安全的设置验证
    bool validate_settings(const SimpleSettings& settings) noexcept;
    SimpleSettings get_default_settings_for_dimension(int dimension) noexcept;
    
    // ✅ 修复: 异常安全的边界工具
    bool validate_bounds(const std::vector<std::pair<double, double>>& bounds) noexcept;
    void print_bounds(const std::vector<std::pair<double, double>>& bounds) noexcept;
    
    // ✅ 修复: 异常安全的结果分析
    void print_optimization_result(const SimpleOptimizationResult& result) noexcept;
    bool save_convergence_history(
        const std::vector<double>& history, 
        const std::string& filename = "convergence_history.csv"
    ) noexcept;
    
    // ✅ 修复: 异常安全的系统信息
    void print_system_info() noexcept;
    int get_recommended_thread_count() noexcept;
}

// 全局配置
namespace Config {
    // 设置全局日志级别
    enum class LogLevel { SILENT, ERROR, WARNING, INFO, DEBUG };
    void set_log_level(LogLevel level) noexcept;
    
    // ✅ 添加: 线程安全的全局配置
    void set_global_random_seed(int seed) noexcept;
    void set_global_thread_count(int count) noexcept;
    void enable_global_caching(bool enable) noexcept;
    
    // 性能调优
    void enable_performance_mode(bool enable) noexcept;
    void set_cache_size(size_t size) noexcept;
    void set_memory_pool_size(size_t size) noexcept;
}

} // namespace OptimizerWrapper
