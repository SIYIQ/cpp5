#include "high_performance_adaptive_de.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <numeric>
#include <execution>

namespace HighPerformanceDE {

// =============================================================================
// AdaptiveParameterManager Implementation
// =============================================================================

AdaptiveParameterManager::AdaptiveParameterManager(int memory_size, int seed) 
    : memory_size_(memory_size), mean_F_(0.5), mean_CR_(0.5), std_F_(0.1), std_CR_(0.1) {
    
    successful_F_.clear();
    successful_CR_.clear();
    
    // 初始化策略成功率
    strategy_success_rates_.resize(5, 0.2); // 5种策略，初始均等概率
    
    // 设置随机数生成器
    if (seed >= 0) {
        rng_.seed(seed);
    } else {
        rng_.seed(std::random_device{}());
    }
}

void AdaptiveParameterManager::add_success(double F, double CR, MutationStrategy strategy) {
    successful_F_.push_back(F);
    successful_CR_.push_back(CR);
    
    // 限制内存使用
    if (successful_F_.size() > memory_size_) {
        successful_F_.pop_front();
        successful_CR_.pop_front();
    }
}

void AdaptiveParameterManager::update_parameters() {
    if (successful_F_.empty()) return;
    
    // 使用Lehmer均值计算F
    double numerator = 0.0, denominator = 0.0;
    for (double f : successful_F_) {
        numerator += f * f;
        denominator += f;
    }
    
    if (denominator > 1e-10) {
        mean_F_ = numerator / denominator;
    }
    
    // 使用算术均值计算CR
    double sum_cr = std::accumulate(successful_CR_.begin(), successful_CR_.end(), 0.0);
    mean_CR_ = sum_cr / successful_CR_.size();
    
    // 限制参数范围
    mean_F_ = std::clamp(mean_F_, 0.1, 2.0);
    mean_CR_ = std::clamp(mean_CR_, 0.0, 1.0);
    
    // 清空成功记录
    successful_F_.clear();
    successful_CR_.clear();
}

std::pair<double, double> AdaptiveParameterManager::generate_parameters() {
    std::normal_distribution<double> f_dist(mean_F_, std_F_);
    std::normal_distribution<double> cr_dist(mean_CR_, std_CR_);
    
    double F = std::clamp(f_dist(rng_), 0.0, 2.0);
    double CR = std::clamp(cr_dist(rng_), 0.0, 1.0);
    
    // 特殊处理F为0的情况
    if (F <= 0.01) {
        F = 0.1 + 0.4 * std::uniform_real_distribution<double>(0.0, 1.0)(rng_);
    }
    
    return {F, CR};
}

MutationStrategy AdaptiveParameterManager::select_strategy() {
    // 计算累积概率
    std::vector<double> cumulative_probs(strategy_success_rates_.size());
    std::partial_sum(strategy_success_rates_.begin(), strategy_success_rates_.end(), 
                     cumulative_probs.begin());
    
    // 归一化
    double total = cumulative_probs.back();
    if (total <= 0.0) {
        return MutationStrategy::RAND_1;  // 默认策略
    }
    
    std::for_each(cumulative_probs.begin(), cumulative_probs.end(), 
                  [total](double& p) { p /= total; });
    
    // 轮盘赌选择
    double rand_val = std::uniform_real_distribution<double>(0.0, 1.0)(rng_);
    
    for (size_t i = 0; i < cumulative_probs.size(); ++i) {
        if (rand_val <= cumulative_probs[i]) {
            return static_cast<MutationStrategy>(i);
        }
    }
    
    return MutationStrategy::RAND_1;
}

void AdaptiveParameterManager::update_strategy_performance(MutationStrategy strategy, bool success) {
    int idx = static_cast<int>(strategy);
    if (idx >= 0 && idx < strategy_success_rates_.size()) {
        const double learning_rate = 0.1;
        const double decay_rate = 0.95;
        
        // 使用指数移动平均更新成功率
        strategy_success_rates_[idx] = decay_rate * strategy_success_rates_[idx] + 
                                      learning_rate * (success ? 1.0 : 0.0);
        
        // 确保最小概率，维持探索
        strategy_success_rates_[idx] = std::max(strategy_success_rates_[idx], 0.05);
    }
}

// =============================================================================
// BoundaryProcessor Implementation  
// =============================================================================

BoundaryProcessor::BoundaryProcessor(const Vector& lower, const Vector& upper, 
                                   BoundaryHandling strategy, int seed)
    : strategy_(strategy), lower_bounds_(lower), upper_bounds_(upper) {
    
    if (seed >= 0) {
        rng_.seed(seed);
    } else {
        rng_.seed(std::random_device{}());
    }
}

void BoundaryProcessor::process(Vector& individual) const {
    const int dim = individual.size();
    
    switch (strategy_) {
        case BoundaryHandling::CLIP:
            for (int i = 0; i < dim; ++i) {
                individual[i] = std::clamp(individual[i], lower_bounds_[i], upper_bounds_[i]);
            }
            break;
            
        case BoundaryHandling::REFLECT:
            for (int i = 0; i < dim; ++i) {
                if (individual[i] < lower_bounds_[i]) {
                    individual[i] = lower_bounds_[i] + (lower_bounds_[i] - individual[i]);
                    individual[i] = std::min(individual[i], upper_bounds_[i]);
                } else if (individual[i] > upper_bounds_[i]) {
                    individual[i] = upper_bounds_[i] - (individual[i] - upper_bounds_[i]);
                    individual[i] = std::max(individual[i], lower_bounds_[i]);
                }
            }
            break;
            
        case BoundaryHandling::REINITIALIZE:
            for (int i = 0; i < dim; ++i) {
                if (individual[i] < lower_bounds_[i] || individual[i] > upper_bounds_[i]) {
                    std::uniform_real_distribution<double> dist(lower_bounds_[i], upper_bounds_[i]);
                    individual[i] = dist(rng_);
                }
            }
            break;
            
        case BoundaryHandling::MIDPOINT:
            for (int i = 0; i < dim; ++i) {
                if (individual[i] < lower_bounds_[i] || individual[i] > upper_bounds_[i]) {
                    individual[i] = (lower_bounds_[i] + upper_bounds_[i]) * 0.5;
                }
            }
            break;
    }
}

void BoundaryProcessor::process_population(std::vector<Individual>& population) const {
    #pragma omp parallel for
    for (size_t i = 0; i < population.size(); ++i) {
        process(population[i].solution);
    }
}

void BoundaryProcessor::process_simd(Vector& individual) const {
    // SIMD优化的边界处理（AVX2）
    const int dim = individual.size();
    double* data = individual.data();
    const double* lower_data = lower_bounds_.data();
    const double* upper_data = upper_bounds_.data();
    
    #ifdef __AVX2__
    const int simd_width = 4;  // AVX2处理4个double
    int simd_end = (dim / simd_width) * simd_width;
    
    for (int i = 0; i < simd_end; i += simd_width) {
        __m256d values = _mm256_load_pd(&data[i]);
        __m256d lower = _mm256_load_pd(&lower_data[i]);
        __m256d upper = _mm256_load_pd(&upper_data[i]);
        
        // SIMD clamp操作
        __m256d clamped = _mm256_max_pd(lower, _mm256_min_pd(values, upper));
        _mm256_store_pd(&data[i], clamped);
    }
    
    // 处理剩余元素
    for (int i = simd_end; i < dim; ++i) {
        data[i] = std::clamp(data[i], lower_data[i], upper_data[i]);
    }
    #else
    // 回退到标准处理
    process(individual);
    #endif
}

// =============================================================================
// SolutionCache Implementation
// =============================================================================

SolutionCache::SolutionCache(size_t max_size, double tolerance) 
    : max_size_(max_size), tolerance_(tolerance) {
    cache_.reserve(max_size);
}

size_t SolutionCache::hash_solution(const Vector& solution) const {
    size_t hash = 0;
    const double* data = solution.data();
    const int size = solution.size();
    
    // 简单但有效的哈希函数
    for (int i = 0; i < size; ++i) {
        // 量化到tolerance_精度
        long long quantized = static_cast<long long>(data[i] / tolerance_);
        hash ^= std::hash<long long>{}(quantized) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    }
    
    return hash;
}

bool SolutionCache::is_similar(const Vector& a, const Vector& b, double tol) const {
    if (a.size() != b.size()) return false;
    
    return (a - b).norm() <= tol;
}

bool SolutionCache::lookup(const Vector& solution, double& fitness) const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    size_t hash_key = hash_solution(solution);
    auto it = cache_.find(hash_key);
    
    if (it != cache_.end()) {
        if (is_similar(solution, it->second.solution, tolerance_)) {
            fitness = it->second.fitness;
            hits_.fetch_add(1, std::memory_order_relaxed);
            return true;
        }
    }
    
    misses_.fetch_add(1, std::memory_order_relaxed);
    return false;
}

void SolutionCache::store(const Vector& solution, double fitness) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    if (cache_.size() >= max_size_) {
        // 简单的LRU策略：删除最旧的条目
        auto oldest = cache_.begin();
        for (auto it = cache_.begin(); it != cache_.end(); ++it) {
            if (it->second.timestamp < oldest->second.timestamp) {
                oldest = it;
            }
        }
        cache_.erase(oldest);
    }
    
    size_t hash_key = hash_solution(solution);
    cache_[hash_key] = {solution, fitness, std::chrono::steady_clock::now()};
}

void SolutionCache::clear() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    cache_.clear();
    hits_.store(0);
    misses_.store(0);
}

double SolutionCache::get_hit_rate() const {
    int total_hits = hits_.load();
    int total_misses = misses_.load();
    int total = total_hits + total_misses;
    
    return total > 0 ? static_cast<double>(total_hits) / total : 0.0;
}

// =============================================================================
// HighPerformanceAdaptiveDE Implementation
// =============================================================================

HighPerformanceAdaptiveDE::HighPerformanceAdaptiveDE(
    ObjectiveFunction objective,
    const Vector& lower_bounds,
    const Vector& upper_bounds,
    const AdaptiveDESettings& settings)
    : objective_function_(std::move(objective)), 
      lower_bounds_(lower_bounds), 
      upper_bounds_(upper_bounds),
      settings_(settings),
      current_generation_(0),
      stagnant_generations_(0),
      total_evaluations_(0) {
    
    // 验证边界
    if (lower_bounds_.size() != upper_bounds_.size()) {
        throw std::invalid_argument("Lower and upper bounds must have the same dimension");
    }
    
    for (int i = 0; i < lower_bounds_.size(); ++i) {
        if (lower_bounds_[i] >= upper_bounds_[i]) {
            throw std::invalid_argument("Lower bound must be less than upper bound");
        }
    }
    
    initialize_components();
}

void HighPerformanceAdaptiveDE::initialize_components() {
    int dimension = lower_bounds_.size();
    
    // 自动计算种群大小
    if (settings_.population_size <= 0) {
        settings_.population_size = std::max(30, 4 * dimension);
        settings_.population_size = std::min(settings_.population_size, 200); // 限制上限
    }
    
    // 初始化随机数生成器
    if (settings_.random_seed >= 0) {
        master_rng_.seed(settings_.random_seed);
    } else {
        master_rng_.seed(std::random_device{}());
    }
    
    // 为每个线程创建独立的随机数生成器
    int num_threads = settings_.num_threads;
    if (num_threads <= 0) {
        num_threads = omp_get_max_threads();
    }
    
    thread_rngs_.resize(num_threads);
    for (int i = 0; i < num_threads; ++i) {
        thread_rngs_[i].seed(master_rng_());
    }
    
    // 初始化自适应组件
    param_manager_ = std::make_unique<AdaptiveParameterManager>(
        settings_.memory_size, master_rng_());
    
    boundary_processor_ = std::make_unique<BoundaryProcessor>(
        lower_bounds_, upper_bounds_, settings_.boundary_handling, master_rng_());
    
    if (settings_.enable_caching) {
        solution_cache_ = std::make_unique<SolutionCache>(10000, 1e-12);
    }
    
    // 预分配内存
    population_.reserve(settings_.population_size);
    if (settings_.use_archive) {
        archive_.reserve(settings_.archive_size);
    }
    convergence_history_.reserve(settings_.max_iterations);
}

void HighPerformanceAdaptiveDE::initialize_population() {
    population_.clear();
    population_.resize(settings_.population_size);
    
    const int dimension = lower_bounds_.size();
    
    // 并行初始化种群
    #pragma omp parallel for
    for (int i = 0; i < settings_.population_size; ++i) {
        int thread_id = omp_get_thread_num();
        auto& rng = thread_rngs_[thread_id];
        
        Vector solution(dimension);
        for (int j = 0; j < dimension; ++j) {
            std::uniform_real_distribution<double> dist(lower_bounds_[j], upper_bounds_[j]);
            solution[j] = dist(rng);
        }
        
        double fitness = evaluate_with_cache(solution);
        population_[i] = Individual(solution, fitness);
    }
    
    // 找到初始最佳个体
    auto best_it = std::min_element(population_.begin(), population_.end(),
        [](const Individual& a, const Individual& b) {
            return a.fitness < b.fitness;
        });
    
    if (best_it != population_.end()) {
        best_individual_ = *best_it;
    }
    
    if (settings_.verbose) {
        std::cout << "种群初始化完成，大小: " << settings_.population_size 
                  << ", 维度: " << dimension
                  << ", 初始最佳适应度: " << best_individual_.fitness << std::endl;
    }
}

Vector HighPerformanceAdaptiveDE::mutate(int target_idx, MutationStrategy strategy, double F) {
    const int pop_size = population_.size();
    const int dimension = lower_bounds_.size();
    Vector mutant(dimension);
    
    // 获取随机个体索引（避免与目标个体相同）
    std::vector<int> candidates;
    candidates.reserve(pop_size - 1);
    for (int i = 0; i < pop_size; ++i) {
        if (i != target_idx) {
            candidates.push_back(i);
        }
    }
    
    int thread_id = omp_get_thread_num();
    auto& rng = thread_rngs_[thread_id % thread_rngs_.size()];
    std::shuffle(candidates.begin(), candidates.end(), rng);
    
    switch (strategy) {
        case MutationStrategy::RAND_1: {
            if (candidates.size() >= 3) {
                int r1 = candidates[0], r2 = candidates[1], r3 = candidates[2];
                mutant = population_[r1].solution + F * (population_[r2].solution - population_[r3].solution);
            }
            break;
        }
        
        case MutationStrategy::BEST_1: {
            if (candidates.size() >= 2) {
                int r1 = candidates[0], r2 = candidates[1];
                mutant = best_individual_.solution + F * (population_[r1].solution - population_[r2].solution);
            }
            break;
        }
        
        case MutationStrategy::CURRENT_TO_BEST_1: {
            if (candidates.size() >= 2) {
                int r1 = candidates[0], r2 = candidates[1];
                mutant = population_[target_idx].solution + 
                        F * (best_individual_.solution - population_[target_idx].solution) +
                        F * (population_[r1].solution - population_[r2].solution);
            }
            break;
        }
        
        case MutationStrategy::RAND_2: {
            if (candidates.size() >= 5) {
                int r1 = candidates[0], r2 = candidates[1], r3 = candidates[2];
                int r4 = candidates[3], r5 = candidates[4];
                mutant = population_[r1].solution + 
                        F * (population_[r2].solution - population_[r3].solution) +
                        F * (population_[r4].solution - population_[r5].solution);
            }
            break;
        }
        
        default:
            // 默认使用RAND_1
            if (candidates.size() >= 3) {
                int r1 = candidates[0], r2 = candidates[1], r3 = candidates[2];
                mutant = population_[r1].solution + F * (population_[r2].solution - population_[r3].solution);
            }
            break;
    }
    
    return mutant;
}

Vector HighPerformanceAdaptiveDE::crossover(const Vector& target, const Vector& mutant, double CR) {
    const int dimension = target.size();
    Vector trial = target;
    
    int thread_id = omp_get_thread_num();
    auto& rng = thread_rngs_[thread_id % thread_rngs_.size()];
    
    std::uniform_real_distribution<double> uniform(0.0, 1.0);
    std::uniform_int_distribution<int> rand_dim(0, dimension - 1);
    
    int forced_dim = rand_dim(rng); // 确保至少一个维度被交叉
    
    for (int i = 0; i < dimension; ++i) {
        if (uniform(rng) < CR || i == forced_dim) {
            trial[i] = mutant[i];
        }
    }
    
    return trial;
}

double HighPerformanceAdaptiveDE::evaluate_with_cache(const Vector& solution) {
    double fitness;
    
    // 尝试从缓存获取
    if (settings_.enable_caching && solution_cache_ && 
        solution_cache_->lookup(solution, fitness)) {
        return fitness;
    }
    
    // 评估目标函数
    fitness = objective_function_(solution);
    total_evaluations_++;
    
    // 存储到缓存
    if (settings_.enable_caching && solution_cache_) {
        solution_cache_->store(solution, fitness);
    }
    
    return fitness;
}

void HighPerformanceAdaptiveDE::parallel_mutation_crossover() {
    const int pop_size = population_.size();
    std::vector<Individual> trial_population(pop_size);
    std::vector<std::pair<double, double>> parameters(pop_size);
    std::vector<MutationStrategy> strategies(pop_size);
    
    // 第一阶段：生成参数和策略
    #pragma omp parallel for
    for (int i = 0; i < pop_size; ++i) {
        parameters[i] = param_manager_->generate_parameters();
        strategies[i] = param_manager_->select_strategy();
    }
    
    // 第二阶段：并行变异和交叉
    #pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < pop_size; ++i) {
        double F = parameters[i].first;
        double CR = parameters[i].second;
        MutationStrategy strategy = strategies[i];
        
        // 变异
        Vector mutant = mutate(i, strategy, F);
        boundary_processor_->process(mutant);
        
        // 交叉
        Vector trial = crossover(population_[i].solution, mutant, CR);
        boundary_processor_->process(trial);
        
        // 创建试验个体
        trial_population[i].solution = std::move(trial);
        trial_population[i].fitness = std::numeric_limits<double>::infinity(); // 稍后评估
    }
    
    // 第三阶段：并行评估
    parallel_evaluation(trial_population);
    
    // 第四阶段：选择和参数更新
    bool improved = false;
    for (int i = 0; i < pop_size; ++i) {
        if (trial_population[i].fitness < population_[i].fitness) {
            // 记录成功参数
            param_manager_->add_success(parameters[i].first, parameters[i].second, strategies[i]);
            param_manager_->update_strategy_performance(strategies[i], true);
            
            // 添加到档案
            if (settings_.use_archive && archive_.size() < settings_.archive_size) {
                archive_.push_back(population_[i]);
            }
            
            // 替换个体
            population_[i] = std::move(trial_population[i]);
            
            // 更新全局最优
            if (population_[i].fitness < best_individual_.fitness) {
                best_individual_ = population_[i];
                improved = true;
                stagnant_generations_ = 0;
            }
        } else {
            param_manager_->update_strategy_performance(strategies[i], false);
        }
    }
    
    if (!improved) {
        stagnant_generations_++;
    }
    
    // 更新参数
    param_manager_->update_parameters();
}

void HighPerformanceAdaptiveDE::parallel_evaluation(std::vector<Individual>& candidates) {
    const int num_candidates = candidates.size();
    
    if (settings_.parallel_evaluation) {
        #pragma omp parallel for schedule(dynamic, 1)
        for (int i = 0; i < num_candidates; ++i) {
            if (candidates[i].fitness == std::numeric_limits<double>::infinity()) {
                candidates[i].fitness = evaluate_with_cache(candidates[i].solution);
            }
        }
    } else {
        // 串行评估
        for (int i = 0; i < num_candidates; ++i) {
            if (candidates[i].fitness == std::numeric_limits<double>::infinity()) {
                candidates[i].fitness = evaluate_with_cache(candidates[i].solution);
            }
        }
    }
}

void HighPerformanceAdaptiveDE::adapt_population_size() {
    if (!settings_.adaptive_population) return;
    
    // 线性种群缩减
    int min_pop_size = std::max(10, static_cast<int>(lower_bounds_.size()));
    int max_pop_size = settings_.population_size;
    
    double progress = static_cast<double>(current_generation_) / settings_.max_iterations;
    int target_size = static_cast<int>(max_pop_size - progress * (max_pop_size - min_pop_size));
    target_size = std::max(target_size, min_pop_size);
    
    if (target_size < population_.size()) {
        // 保留最优个体
        std::partial_sort(population_.begin(), population_.begin() + target_size, 
                         population_.end(),
                         [](const Individual& a, const Individual& b) {
                             return a.fitness < b.fitness;
                         });
        
        population_.resize(target_size);
        
        if (settings_.verbose && current_generation_ % 100 == 0) {
            std::cout << "种群大小调整为: " << target_size << std::endl;
        }
    }
}

bool HighPerformanceAdaptiveDE::check_convergence() {
    // 适应度容忍度检查
    if (std::abs(best_individual_.fitness) < settings_.tolerance) {
        return true;
    }
    
    // 停滞检查
    if (stagnant_generations_ >= settings_.max_stagnant_generations) {
        return true;
    }
    
    // 种群多样性检查
    if (current_generation_ > 100) {
        double diversity = Utils::calculate_diversity(population_);
        if (diversity < 1e-10) {
            return true;
        }
    }
    
    return false;
}

void HighPerformanceAdaptiveDE::print_generation_info() {
    if (!settings_.verbose) return;
    
    if (current_generation_ % 50 == 0 || current_generation_ == 1) {
        auto [mean_F, mean_CR] = param_manager_->get_current_means();
        const auto& strategy_rates = param_manager_->get_strategy_rates();
        
        std::cout << "代数 " << std::setw(4) << current_generation_ 
                  << ": 最佳适应度 = " << std::scientific << std::setprecision(6) << best_individual_.fitness
                  << ", F = " << std::fixed << std::setprecision(3) << mean_F
                  << ", CR = " << mean_CR
                  << ", 种群 = " << population_.size();
        
        if (settings_.enable_caching && solution_cache_) {
            std::cout << ", 缓存命中率 = " << std::setprecision(1) 
                     << (solution_cache_->get_hit_rate() * 100) << "%";
        }
        
        std::cout << std::endl;
    }
}

OptimizationResult HighPerformanceAdaptiveDE::optimize() {
    start_time_ = std::chrono::steady_clock::now();
    
    // 初始化
    initialize_population();
    
    if (settings_.verbose) {
        std::cout << "开始自适应差分进化优化..." << std::endl;
        std::cout << "设置: 种群=" << population_.size() 
                  << ", 最大代数=" << settings_.max_iterations
                  << ", 维度=" << lower_bounds_.size() 
                  << ", 并行线程=" << (settings_.num_threads > 0 ? settings_.num_threads : omp_get_max_threads())
                  << std::endl;
    }
    
    // 主进化循环
    for (current_generation_ = 1; current_generation_ <= settings_.max_iterations; ++current_generation_) {
        // 并行变异、交叉和选择
        parallel_mutation_crossover();
        
        // 自适应种群大小
        adapt_population_size();
        
        // 记录收敛历史
        convergence_history_.push_back(best_individual_.fitness);
        
        // 打印进度
        print_generation_info();
        
        // 收敛检查
        if (check_convergence()) {
            if (settings_.verbose) {
                std::cout << "在第 " << current_generation_ << " 代收敛" << std::endl;
            }
            break;
        }
    }
    
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time_);
    
    // 创建优化结果
    OptimizationResult result;
    result.best_solution = best_individual_.solution;
    result.best_fitness = best_individual_.fitness;
    result.iterations = current_generation_;
    result.execution_time = duration.count() / 1000.0;
    result.converged = (std::abs(best_individual_.fitness) < settings_.tolerance);
    result.convergence_history = convergence_history_;
    
    // 性能统计
    result.performance_stats.total_evaluations = total_evaluations_;
    result.performance_stats.avg_evaluation_time = result.execution_time / total_evaluations_;
    
    if (settings_.enable_caching && solution_cache_) {
        auto [hits, misses] = solution_cache_->get_statistics();
        result.performance_stats.cache_hits = hits;
        result.performance_stats.cache_misses = misses;
    }
    
    if (settings_.verbose) {
        print_performance_report();
    }
    
    return result;
}

void HighPerformanceAdaptiveDE::print_performance_report() const {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "高性能自适应DE优化完成" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    std::cout << "最优解: ";
    for (int i = 0; i < std::min(5, static_cast<int>(best_individual_.solution.size())); ++i) {
        std::cout << best_individual_.solution[i] << " ";
    }
    if (best_individual_.solution.size() > 5) std::cout << "...";
    std::cout << std::endl;
    
    std::cout << "最优值: " << std::scientific << best_individual_.fitness << std::endl;
    std::cout << "迭代次数: " << current_generation_ << std::endl;
    std::cout << "函数评估次数: " << total_evaluations_ << std::endl;
    
    if (settings_.enable_caching && solution_cache_) {
        std::cout << "缓存命中率: " << std::fixed << std::setprecision(1) 
                 << (solution_cache_->get_hit_rate() * 100) << "%" << std::endl;
    }
    
    auto [mean_F, mean_CR] = param_manager_->get_current_means();
    std::cout << "最终参数: F=" << std::setprecision(3) << mean_F 
              << ", CR=" << mean_CR << std::endl;
    
    const auto& strategy_rates = param_manager_->get_strategy_rates();
    std::cout << "策略成功率: ";
    const std::vector<std::string> strategy_names = {
        "RAND_1", "BEST_1", "CURR_TO_BEST", "RAND_2", "BEST_2"
    };
    for (size_t i = 0; i < strategy_rates.size() && i < strategy_names.size(); ++i) {
        std::cout << strategy_names[i] << "=" << std::setprecision(2) << strategy_rates[i] << " ";
    }
    std::cout << std::endl;
}

// =============================================================================
// 工厂方法和便利函数
// =============================================================================

std::unique_ptr<HighPerformanceAdaptiveDE> HighPerformanceAdaptiveDE::create_for_problem_size(
    ObjectiveFunction objective,
    const Vector& lower_bounds, 
    const Vector& upper_bounds,
    int problem_dimension) {
    
    AdaptiveDESettings settings;
    
    // 根据问题规模调整参数
    if (problem_dimension < 10) {
        settings.population_size = std::max(30, 4 * problem_dimension);
        settings.max_iterations = 500;
    } else if (problem_dimension < 30) {
        settings.population_size = std::max(60, 6 * problem_dimension);
        settings.max_iterations = 800;
    } else if (problem_dimension < 100) {
        settings.population_size = std::min(200, 10 * problem_dimension);
        settings.max_iterations = 1200;
    } else {
        settings.population_size = 300;
        settings.max_iterations = 2000;
    }
    
    settings.adaptive_population = true;
    settings.use_archive = true;
    settings.parallel_evaluation = true;
    settings.enable_caching = true;
    
    return std::make_unique<HighPerformanceAdaptiveDE>(
        std::move(objective), lower_bounds, upper_bounds, settings
    );
}

OptimizationResult adaptive_differential_evolution(
    ObjectiveFunction objective,
    const std::vector<std::pair<double, double>>& bounds,
    const AdaptiveDESettings& settings) {
    
    Vector lower_bounds = Utils::bounds_to_lower(bounds);
    Vector upper_bounds = Utils::bounds_to_upper(bounds);
    
    HighPerformanceAdaptiveDE optimizer(std::move(objective), lower_bounds, upper_bounds, settings);
    return optimizer.optimize();
}

// =============================================================================
// 实用工具函数
// =============================================================================

namespace Utils {

Vector bounds_to_lower(const std::vector<std::pair<double, double>>& bounds) {
    Vector lower(bounds.size());
    for (size_t i = 0; i < bounds.size(); ++i) {
        lower[i] = bounds[i].first;
    }
    return lower;
}

Vector bounds_to_upper(const std::vector<std::pair<double, double>>& bounds) {
    Vector upper(bounds.size());
    for (size_t i = 0; i < bounds.size(); ++i) {
        upper[i] = bounds[i].second;
    }
    return upper;
}

void print_vector(const Vector& v, const std::string& name) {
    std::cout << name << ": [";
    for (int i = 0; i < v.size(); ++i) {
        std::cout << v[i];
        if (i < v.size() - 1) std::cout << ", ";
    }
    std::cout << "]" << std::endl;
}

double calculate_diversity(const std::vector<Individual>& population) {
    if (population.empty()) return 0.0;
    
    double total_distance = 0.0;
    int count = 0;
    
    const int pop_size = population.size();
    for (int i = 0; i < pop_size; ++i) {
        for (int j = i + 1; j < pop_size; ++j) {
            total_distance += (population[i].solution - population[j].solution).norm();
            count++;
        }
    }
    
    return count > 0 ? total_distance / count : 0.0;
}

std::string format_time(double seconds) {
    std::ostringstream oss;
    if (seconds < 60) {
        oss << std::fixed << std::setprecision(2) << seconds << "s";
    } else if (seconds < 3600) {
        int minutes = static_cast<int>(seconds / 60);
        double remaining_seconds = seconds - minutes * 60;
        oss << minutes << "m " << std::fixed << std::setprecision(1) << remaining_seconds << "s";
    } else {
        int hours = static_cast<int>(seconds / 3600);
        int minutes = static_cast<int>((seconds - hours * 3600) / 60);
        oss << hours << "h " << minutes << "m";
    }
    return oss.str();
}

} // namespace Utils

} // namespace HighPerformanceDE
