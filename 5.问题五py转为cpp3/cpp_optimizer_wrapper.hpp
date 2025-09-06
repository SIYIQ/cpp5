#pragma once

#include "high_performance_adaptive_de.hpp"
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

namespace OptimizerWrapper {

// 前向声明
class Problem5Objective;

// 优化结果的简化版本，便于与Python交互
struct SimpleOptimizationResult {
    std::vector<double> best_solution;
    double best_fitness;
    int iterations;
    double execution_time;
    bool converged;
    std::vector<double> convergence_history;
    
    // 性能统计
    size_t total_evaluations;
    double cache_hit_rate;
    std::vector<double> strategy_success_rates;
    std::vector<double> final_parameters; // [mean_F, mean_CR]
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
    
    // 转换为内部设置
    HighPerformanceDE::AdaptiveDESettings to_internal_settings() const;
};

// 问题5专用的C++优化器包装类
class Problem5CppOptimizer {
private:
    std::string missile_id_;
    std::unordered_map<std::string, int> uav_assignments_;
    std::unique_ptr<Problem5Objective> objective_;
    std::vector<std::pair<double, double>> bounds_;
    int dimension_;
    
    // 性能统计
    mutable double last_optimization_time_ = 0.0;
    mutable size_t last_total_evaluations_ = 0;
    
public:
    // 构造函数
    explicit Problem5CppOptimizer(
        const std::string& missile_id,
        const std::unordered_map<std::string, int>& uav_assignments
    );
    
    ~Problem5CppOptimizer();
    
    // 设置优化边界
    void set_bounds(const std::vector<std::pair<double, double>>& bounds);
    
    // 主要优化接口
    SimpleOptimizationResult optimize(const SimpleSettings& settings = SimpleSettings());
    
    // 性能测试接口
    SimpleOptimizationResult benchmark_against_scipy(
        const SimpleSettings& settings,
        int num_runs = 3
    );
    
    // 获取推荐设置
    SimpleSettings get_recommended_settings() const;
    
    // 工具函数
    int get_dimension() const { return dimension_; }
    double get_last_optimization_time() const { return last_optimization_time_; }
    size_t get_last_evaluations() const { return last_total_evaluations_; }
    
    // 静态工厂方法
    static std::unique_ptr<Problem5CppOptimizer> create(
        const std::string& missile_id,
        const std::unordered_map<std::string, int>& uav_assignments,
        const std::vector<std::pair<double, double>>& bounds
    );
};

// 目标函数包装类（连接到原有的Python逻辑）
class Problem5Objective {
private:
    std::string missile_id_;
    std::unordered_map<std::string, int> uav_assignments_;
    
    // 缓存Python对象的指针（如果需要的话）
    void* python_objective_ptr_ = nullptr;
    
    // 决策变量解析
    struct UAVStrategy {
        double speed;
        double angle;
        std::vector<std::pair<double, double>> grenades; // {t_deploy, t_fuse}
    };
    
    std::unordered_map<std::string, UAVStrategy> parse_decision_variables(
        const std::vector<double>& decision_variables
    ) const;
    
    // 核心目标函数计算（重新实现Python逻辑）
    double calculate_obscuration_time(
        const std::unordered_map<std::string, UAVStrategy>& strategies
    ) const;
    
    // 约束检查
    double calculate_constraint_violation(
        const std::vector<double>& decision_variables
    ) const;
    
public:
    Problem5Objective(
        const std::string& missile_id,
        const std::unordered_map<std::string, int>& uav_assignments
    );
    
    ~Problem5Objective();
    
    // 主要目标函数接口
    double operator()(const HighPerformanceDE::Vector& x) const;
    
    // 获取统计信息
    struct Statistics {
        size_t total_calls = 0;
        size_t constraint_violations = 0;
        double avg_evaluation_time = 0.0;
        double best_fitness_seen = std::numeric_limits<double>::infinity();
    };
    
    mutable Statistics stats_;
    const Statistics& get_statistics() const { return stats_; }
    void reset_statistics() { stats_ = Statistics{}; }
};

// 性能基准测试工具
namespace Benchmark {
    
    struct ComparisonResult {
        std::string algorithm_name;
        double avg_time;
        double avg_fitness;
        double std_fitness;
        double best_fitness;
        int avg_iterations;
        double success_rate;
        
        // 相对于基准的改进
        double time_improvement_percent = 0.0;
        double fitness_improvement_percent = 0.0;
    };
    
    // 对比不同算法
    std::vector<ComparisonResult> compare_algorithms(
        const std::string& missile_id,
        const std::unordered_map<std::string, int>& uav_assignments,
        const std::vector<std::pair<double, double>>& bounds,
        int num_runs = 5
    );
    
    // 扩展性测试
    void test_parallel_scaling(
        const std::string& missile_id,
        const std::unordered_map<std::string, int>& uav_assignments,
        const std::vector<std::pair<double, double>>& bounds
    );
    
    // 内存使用分析
    void analyze_memory_usage(
        const std::string& missile_id,
        const std::unordered_map<std::string, int>& uav_assignments,
        const std::vector<std::pair<double, double>>& bounds
    );
    
    // 生成性能报告
    void generate_performance_report(
        const std::vector<ComparisonResult>& results,
        const std::string& output_file = "cpp_performance_report.html"
    );
}

// 便利函数
namespace Utils {
    
    // Python兼容性工具
    std::vector<double> eigen_to_vector(const HighPerformanceDE::Vector& v);
    HighPerformanceDE::Vector vector_to_eigen(const std::vector<double>& v);
    
    // 设置验证
    bool validate_settings(const SimpleSettings& settings);
    SimpleSettings get_default_settings_for_dimension(int dimension);
    
    // 边界工具
    bool validate_bounds(const std::vector<std::pair<double, double>>& bounds);
    void print_bounds(const std::vector<std::pair<double, double>>& bounds);
    
    // 结果分析
    void print_optimization_result(const SimpleOptimizationResult& result);
    void save_convergence_history(
        const std::vector<double>& history, 
        const std::string& filename = "convergence_history.csv"
    );
    
    // 系统信息
    void print_system_info();
    int get_recommended_thread_count();
}

// 全局配置
namespace Config {
    // 设置全局日志级别
    enum class LogLevel { SILENT, ERROR, WARNING, INFO, DEBUG };
    void set_log_level(LogLevel level);
    
    // 设置全局并行策略
    void set_parallel_strategy(const std::string& strategy); // "openmp", "tbb", "std_async"
    
    // 性能调优
    void enable_performance_mode(bool enable);
    void set_cache_size(size_t size);
    void set_memory_pool_size(size_t size);
}

} // namespace OptimizerWrapper
