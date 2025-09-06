#pragma once

#include <vector>
#include <functional>
#include <random>
#include <memory>
#include <atomic>
#include <future>
#include <deque>
#include <unordered_map>
#include <algorithm>
#include <chrono>
#include <immintrin.h>  // AVX2 SIMD support
#include <omp.h>
#include <Eigen/Dense>

namespace HighPerformanceDE {

using Vector = Eigen::VectorXd;
using Matrix = Eigen::MatrixXd;
using ObjectiveFunction = std::function<double(const Vector&)>;

// 变异策略枚举
enum class MutationStrategy {
    RAND_1,             // DE/rand/1
    BEST_1,             // DE/best/1  
    CURRENT_TO_BEST_1,  // DE/current-to-best/1
    RAND_2,             // DE/rand/2
    BEST_2              // DE/best/2
};

// 边界处理策略
enum class BoundaryHandling {
    CLIP,           // 截断
    REFLECT,        // 反射  
    REINITIALIZE,   // 重新初始化
    MIDPOINT        // 中点修正
};

// 优化结果结构
struct OptimizationResult {
    Vector best_solution;
    double best_fitness;
    int iterations;
    double execution_time;
    bool converged;
    std::vector<double> convergence_history;
    
    // 性能统计
    struct PerformanceStats {
        size_t total_evaluations;
        double avg_evaluation_time;
        double parallel_efficiency;
        int cache_hits;
        int cache_misses;
    } performance_stats;
};

// 算法设置
struct AdaptiveDESettings {
    int population_size = 0;           // 0表示自动计算
    int max_iterations = 1000;
    double tolerance = 1e-6;
    int max_stagnant_generations = 50;
    bool adaptive_population = true;   // 动态种群大小
    bool use_archive = true;          // 使用历史档案
    int archive_size = 100;
    BoundaryHandling boundary_handling = BoundaryHandling::REFLECT;
    int random_seed = -1;             // -1表示随机种子
    bool parallel_evaluation = true;  // 并行评估
    int num_threads = -1;             // -1表示使用所有可用线程
    bool use_simd = true;             // 使用SIMD优化
    bool enable_caching = true;       // 启用解缓存
    bool verbose = true;
    
    // 自适应参数
    int memory_size = 100;            // 成功参数记忆大小
    double learning_rate = 0.1;       // 参数学习率
    bool strategy_adaptation = true;   // 策略自适应
};

// 内存对齐的个体结构，优化缓存访问
struct alignas(64) Individual {
    Vector solution;
    double fitness;
    double constraint_violation;
    int age;  // 个体年龄，用于多样性维护
    
    Individual() : fitness(std::numeric_limits<double>::infinity()), constraint_violation(0.0), age(0) {}
    Individual(const Vector& sol, double fit) : solution(sol), fitness(fit), constraint_violation(0.0), age(0) {}
};

// 高性能参数自适应管理器
class AdaptiveParameterManager {
private:
    std::deque<double> successful_F_;
    std::deque<double> successful_CR_;
    std::vector<double> strategy_success_rates_;
    double mean_F_;
    double mean_CR_;
    double std_F_;
    double std_CR_;
    int memory_size_;
    std::mt19937 rng_;
    
public:
    explicit AdaptiveParameterManager(int memory_size = 100, int seed = -1);
    
    void add_success(double F, double CR, MutationStrategy strategy);
    void update_parameters();
    std::pair<double, double> generate_parameters();
    MutationStrategy select_strategy();
    void update_strategy_performance(MutationStrategy strategy, bool success);
    
    // 获取当前参数统计
    std::pair<double, double> get_current_means() const { return {mean_F_, mean_CR_}; }
    const std::vector<double>& get_strategy_rates() const { return strategy_success_rates_; }
};

// 高性能边界处理器
class BoundaryProcessor {
private:
    BoundaryHandling strategy_;
    Vector lower_bounds_;
    Vector upper_bounds_;
    std::mt19937 rng_;
    
public:
    BoundaryProcessor(const Vector& lower, const Vector& upper, 
                     BoundaryHandling strategy = BoundaryHandling::REFLECT, int seed = -1);
    
    void process(Vector& individual) const;
    void process_population(std::vector<Individual>& population) const;
    
    // SIMD优化的边界处理
    void process_simd(Vector& individual) const;
};

// 解缓存系统，避免重复评估
class SolutionCache {
private:
    struct CacheEntry {
        Vector solution;
        double fitness;
        std::chrono::steady_clock::time_point timestamp;
    };
    
    std::unordered_map<size_t, CacheEntry> cache_;
    mutable std::mutex cache_mutex_;
    size_t max_size_;
    double tolerance_;
    std::atomic<int> hits_{0};
    std::atomic<int> misses_{0};
    
    size_t hash_solution(const Vector& solution) const;
    bool is_similar(const Vector& a, const Vector& b, double tol) const;
    
public:
    explicit SolutionCache(size_t max_size = 10000, double tolerance = 1e-10);
    
    bool lookup(const Vector& solution, double& fitness) const;
    void store(const Vector& solution, double fitness);
    void clear();
    
    std::pair<int, int> get_statistics() const { return {hits_.load(), misses_.load()}; }
    double get_hit_rate() const;
};

// 高性能自适应差分进化主类
class HighPerformanceAdaptiveDE {
private:
    // 核心组件
    ObjectiveFunction objective_function_;
    Vector lower_bounds_;
    Vector upper_bounds_;
    AdaptiveDESettings settings_;
    
    // 算法状态
    std::vector<Individual> population_;
    std::vector<Individual> archive_;  // 历史档案
    Individual best_individual_;
    int current_generation_;
    int stagnant_generations_;
    
    // 自适应组件
    std::unique_ptr<AdaptiveParameterManager> param_manager_;
    std::unique_ptr<BoundaryProcessor> boundary_processor_;
    std::unique_ptr<SolutionCache> solution_cache_;
    
    // 性能优化
    std::mt19937 master_rng_;
    std::vector<std::mt19937> thread_rngs_;  // 每线程独立随机数生成器
    
    // 统计信息
    std::chrono::steady_clock::time_point start_time_;
    size_t total_evaluations_;
    std::vector<double> convergence_history_;
    
    // 私有方法
    void initialize_population();
    void initialize_components();
    Vector mutate(int target_idx, MutationStrategy strategy, double F);
    Vector crossover(const Vector& target, const Vector& mutant, double CR);
    double evaluate_with_cache(const Vector& solution);
    void selection_step();
    void update_archive();
    void adapt_population_size();
    bool check_convergence();
    void print_generation_info();
    
    // 高性能并行方法
    void parallel_mutation_crossover();
    void parallel_evaluation(std::vector<Individual>& candidates);
    
    // SIMD优化方法
    void simd_vector_operations();
    
public:
    explicit HighPerformanceAdaptiveDE(
        ObjectiveFunction objective,
        const Vector& lower_bounds,
        const Vector& upper_bounds,
        const AdaptiveDESettings& settings = AdaptiveDESettings()
    );
    
    ~HighPerformanceAdaptiveDE() = default;
    
    // 禁用拷贝，移动构造
    HighPerformanceAdaptiveDE(const HighPerformanceAdaptiveDE&) = delete;
    HighPerformanceAdaptiveDE& operator=(const HighPerformanceAdaptiveDE&) = delete;
    HighPerformanceAdaptiveDE(HighPerformanceAdaptiveDE&&) = default;
    HighPerformanceAdaptiveDE& operator=(HighPerformanceAdaptiveDE&&) = default;
    
    // 主要优化接口
    OptimizationResult optimize();
    
    // 获取当前状态
    const Individual& get_best_individual() const { return best_individual_; }
    const std::vector<Individual>& get_population() const { return population_; }
    const std::vector<double>& get_convergence_history() const { return convergence_history_; }
    
    // 性能分析接口
    void enable_profiling(bool enable);
    void print_performance_report() const;
    
    // 参数调优建议
    void suggest_parameters() const;
    
    // 静态工厂方法
    static std::unique_ptr<HighPerformanceAdaptiveDE> create_for_problem_size(
        ObjectiveFunction objective,
        const Vector& lower_bounds, 
        const Vector& upper_bounds,
        int problem_dimension
    );
};

// 便利函数
OptimizationResult adaptive_differential_evolution(
    ObjectiveFunction objective,
    const std::vector<std::pair<double, double>>& bounds,
    const AdaptiveDESettings& settings = AdaptiveDESettings()
);

// 性能基准测试
namespace Benchmark {
    void run_performance_tests();
    void compare_with_standard_de();
    void profile_memory_usage();
    void test_parallel_scaling();
}

// 实用工具
namespace Utils {
    Vector bounds_to_lower(const std::vector<std::pair<double, double>>& bounds);
    Vector bounds_to_upper(const std::vector<std::pair<double, double>>& bounds);
    void print_vector(const Vector& v, const std::string& name = "Vector");
    double calculate_diversity(const std::vector<Individual>& population);
    std::string format_time(double seconds);
}

} // namespace HighPerformanceDE
