// 修复后的高性能自适应差分进化算法实现 (关键部分)
// 此文件包含主要Bug修复，完整版本请参考原文件

#include "high_performance_adaptive_de.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <numeric>
#include <execution>
#include <stdexcept>

namespace HighPerformanceDE {

// =============================================================================
// AdaptiveParameterManager Implementation (修复版本)
// =============================================================================

void AdaptiveParameterManager::update_parameters() {
    if (successful_F_.empty()) return;
    
    // 修复: 添加数值稳定性检查
    double numerator = 0.0, denominator = 0.0;
    for (double f : successful_F_) {
        numerator += f * f;
        denominator += f;
    }
    
    // ✅ 修复: 防止除零错误
    const double epsilon = 1e-12;
    if (std::abs(denominator) > epsilon) {
        mean_F_ = numerator / denominator;
    } else {
        mean_F_ = 0.5;  // 回退到安全默认值
    }
    
    // ✅ 修复: 数值稳定性检查
    if (!successful_CR_.empty()) {
        double sum_cr = std::accumulate(successful_CR_.begin(), successful_CR_.end(), 0.0);
        mean_CR_ = sum_cr / successful_CR_.size();
    }
    
    // 限制参数范围
    mean_F_ = std::clamp(mean_F_, 0.1, 2.0);
    mean_CR_ = std::clamp(mean_CR_, 0.0, 1.0);
    
    // 清空成功记录
    successful_F_.clear();
    successful_CR_.clear();
}

MutationStrategy AdaptiveParameterManager::select_strategy() {
    // ✅ 修复: 添加安全检查
    if (strategy_success_rates_.empty()) {
        return MutationStrategy::RAND_1;  // 安全默认值
    }
    
    // 计算累积概率
    std::vector<double> cumulative_probs(strategy_success_rates_.size());
    std::partial_sum(strategy_success_rates_.begin(), strategy_success_rates_.end(), 
                     cumulative_probs.begin());
    
    double total = cumulative_probs.back();
    // ✅ 修复: 浮点数比较
    const double epsilon = 1e-12;
    if (total <= epsilon) {
        return MutationStrategy::RAND_1;  // 安全默认值
    }
    
    // 归一化
    for (double& p : cumulative_probs) {
        p /= total;
    }
    
    // 轮盘赌选择
    double rand_val = std::uniform_real_distribution<double>(0.0, 1.0)(rng_);
    
    for (size_t i = 0; i < cumulative_probs.size(); ++i) {
        if (rand_val <= cumulative_probs[i]) {
            return static_cast<MutationStrategy>(i);
        }
    }
    
    return MutationStrategy::RAND_1;  // 回退保护
}

// =============================================================================
// BoundaryProcessor Implementation (修复版本)
// =============================================================================

void BoundaryProcessor::process_simd(Vector& individual) const {
    const int dim = individual.size();
    
    // ✅ 修复: 添加边界检查
    if (dim == 0) return;
    if (dim != lower_bounds_.size() || dim != upper_bounds_.size()) {
        throw std::invalid_argument("向量维度不匹配边界维度");
    }
    
    double* data = individual.data();
    const double* lower_data = lower_bounds_.data();
    const double* upper_data = upper_bounds_.data();
    
    // ✅ 修复: 安全的SIMD处理
    #ifdef __AVX2__
    // 检查是否值得使用SIMD (至少4个元素)
    if (dim >= 4) {
        const int simd_width = 4;
        const int simd_end = (dim / simd_width) * simd_width;
        
        // ✅ 修复: 使用未对齐加载，避免内存对齐要求
        for (int i = 0; i < simd_end; i += simd_width) {
            // 检查不会越界
            if (i + simd_width <= dim) {
                __m256d values = _mm256_loadu_pd(&data[i]);        // 未对齐加载
                __m256d lower = _mm256_loadu_pd(&lower_data[i]);   // 未对齐加载
                __m256d upper = _mm256_loadu_pd(&upper_data[i]);   // 未对齐加载
                __m256d clamped = _mm256_max_pd(lower, _mm256_min_pd(values, upper));
                _mm256_storeu_pd(&data[i], clamped);               // 未对齐存储
            }
        }
        
        // 处理剩余元素
        for (int i = simd_end; i < dim; ++i) {
            data[i] = std::clamp(data[i], lower_data[i], upper_data[i]);
        }
        return;
    }
    #endif
    
    // ✅ 修复: 回退到安全的标准处理
    for (int i = 0; i < dim; ++i) {
        individual[i] = std::clamp(individual[i], lower_bounds_[i], upper_bounds_[i]);
    }
}

// =============================================================================
// HighPerformanceAdaptiveDE Implementation (修复版本)
// =============================================================================

void HighPerformanceAdaptiveDE::initialize_components() {
    int dimension = lower_bounds_.size();
    
    // ✅ 修复: 添加维度检查
    if (dimension <= 0) {
        throw std::invalid_argument("优化维度必须大于0");
    }
    
    // 自动计算种群大小
    if (settings_.population_size <= 0) {
        settings_.population_size = std::max(30, 4 * dimension);
        settings_.population_size = std::min(settings_.population_size, 200);
    }
    
    // 初始化随机数生成器
    if (settings_.random_seed >= 0) {
        master_rng_.seed(settings_.random_seed);
    } else {
        master_rng_.seed(std::random_device{}());
    }
    
    // ✅ 修复: 安全的线程随机数生成器初始化
    int num_threads = settings_.num_threads;
    if (num_threads <= 0) {
        num_threads = std::max(1, static_cast<int>(omp_get_max_threads()));
    }
    
    thread_rngs_.clear();  // 清空现有的
    thread_rngs_.reserve(num_threads);
    for (int i = 0; i < num_threads; ++i) {
        thread_rngs_.emplace_back(master_rng_());
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

Vector HighPerformanceAdaptiveDE::mutate(int target_idx, MutationStrategy strategy, double F) {
    const int pop_size = population_.size();
    const int dimension = lower_bounds_.size();
    
    // ✅ 修复: 参数有效性检查
    if (target_idx < 0 || target_idx >= pop_size) {
        throw std::out_of_range("目标个体索引超出范围");
    }
    if (dimension <= 0) {
        throw std::invalid_argument("优化维度无效");
    }
    
    Vector mutant(dimension);
    
    // 获取随机个体索引
    std::vector<int> candidates;
    candidates.reserve(pop_size - 1);
    for (int i = 0; i < pop_size; ++i) {
        if (i != target_idx) {
            candidates.push_back(i);
        }
    }
    
    // ✅ 修复: 检查是否有足够的候选个体
    if (candidates.size() < 3) {
        throw std::runtime_error("种群大小不足以进行变异操作");
    }
    
    // ✅ 修复: 安全的随机数生成器访问
    int thread_id = omp_get_thread_num();
    if (thread_rngs_.empty()) {
        throw std::runtime_error("随机数生成器未初始化");
    }
    auto& rng = thread_rngs_[thread_id % thread_rngs_.size()];
    
    std::shuffle(candidates.begin(), candidates.end(), rng);
    
    // 执行变异策略
    switch (strategy) {
        case MutationStrategy::RAND_1: {
            if (candidates.size() >= 3) {
                int r1 = candidates[0], r2 = candidates[1], r3 = candidates[2];
                mutant = population_[r1].solution + F * (population_[r2].solution - population_[r3].solution);
            } else {
                // ✅ 修复: 回退策略
                mutant = population_[target_idx].solution;
            }
            break;
        }
        
        case MutationStrategy::BEST_1: {
            if (candidates.size() >= 2) {
                int r1 = candidates[0], r2 = candidates[1];
                mutant = best_individual_.solution + F * (population_[r1].solution - population_[r2].solution);
            } else {
                mutant = best_individual_.solution;
            }
            break;
        }
        
        case MutationStrategy::CURRENT_TO_BEST_1: {
            if (candidates.size() >= 2) {
                int r1 = candidates[0], r2 = candidates[1];
                mutant = population_[target_idx].solution + 
                        F * (best_individual_.solution - population_[target_idx].solution) +
                        F * (population_[r1].solution - population_[r2].solution);
            } else {
                mutant = population_[target_idx].solution;
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
            } else {
                // 回退到RAND_1
                if (candidates.size() >= 3) {
                    int r1 = candidates[0], r2 = candidates[1], r3 = candidates[2];
                    mutant = population_[r1].solution + F * (population_[r2].solution - population_[r3].solution);
                } else {
                    mutant = population_[target_idx].solution;
                }
            }
            break;
        }
        
        default:
            mutant = population_[target_idx].solution;  // 安全回退
            break;
    }
    
    return mutant;
}

// ✅ 修复: 添加数值稳定性检查
bool HighPerformanceAdaptiveDE::check_convergence() {
    const double epsilon = 1e-12;
    
    // 适应度容忍度检查
    if (std::abs(best_individual_.fitness) < settings_.tolerance) {
        return true;
    }
    
    // 停滞检查
    if (stagnant_generations_ >= settings_.max_stagnant_generations) {
        return true;
    }
    
    // 种群多样性检查 (修复: 添加数值稳定性)
    if (current_generation_ > 100 && population_.size() > 1) {
        double diversity = Utils::calculate_diversity(population_);
        if (diversity < epsilon) {
            return true;
        }
    }
    
    return false;
}

} // namespace HighPerformanceDE
