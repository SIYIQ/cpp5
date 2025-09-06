#include "optimizer.hpp"
#include <iostream>
#include <algorithm>
#include <limits>
#include <cmath>
#include <set>
#include <omp.h>

namespace Optimizer {

// ObscurationOptimizer Implementation
ObscurationOptimizer::ObscurationOptimizer(
    const std::string& missile_id, 
    const std::unordered_map<std::string, int>& uav_assignments)
    : uav_assignments_(uav_assignments)
    , time_step_(0.1)
    , rng_(std::random_device{}())
{
    missile_ = std::make_unique<CoreObjects::Missile>(missile_id);
    target_ = std::make_unique<CoreObjects::TargetCylinder>(Config::TRUE_TARGET_SPECS);
    target_key_points_ = target_->get_key_points();
}

std::pair<StrategyMap, double> ObscurationOptimizer::solve(
    const std::vector<Bounds>& bounds, 
    const DESettings& settings)
{
    auto [optimal_vars, max_time] = differential_evolution(bounds, settings);
    
    // 重新构建最优策略用于详细输出
    StrategyMap optimal_strategy = parse_decision_variables(optimal_vars);
    return {optimal_strategy, -max_time}; // 注意取负号，因为我们最小化负值
}

double ObscurationOptimizer::objective_function(const VectorXd& decision_variables) {
    try {
        StrategyMap strategies = parse_decision_variables(decision_variables);
        
        std::vector<std::unique_ptr<CoreObjects::SmokeCloud>> smoke_clouds;
        
        for (const auto& [uav_id, uav_strat] : strategies) {
            auto uav = std::make_unique<CoreObjects::UAV>(uav_id);
            uav->set_flight_strategy(uav_strat.speed, uav_strat.angle);
            
            for (const auto& g_strat : uav_strat.grenades) {
                auto grenade = uav->deploy_grenade(g_strat.t_deploy, g_strat.t_fuse);
                smoke_clouds.push_back(grenade->generate_smoke_cloud());
            }
        }
        
        if (smoke_clouds.empty()) {
            return 0.0;
        }
        
        // 计算仿真时间范围
        double sim_start_time = std::numeric_limits<double>::max();
        double sim_end_time = std::numeric_limits<double>::lowest();
        
        for (const auto& cloud : smoke_clouds) {
            sim_start_time = std::min(sim_start_time, cloud->get_start_time());
            sim_end_time = std::max(sim_end_time, cloud->get_end_time());
        }
        
        // 使用集合去重计算有效遮蔽时间点
        std::set<int> obscured_time_points;
        
        for (double t = sim_start_time; t < sim_end_time; t += time_step_) {
            // 获取当前所有有效的云团中心
            std::vector<Vector3d> active_cloud_centers;
            
            for (const auto& cloud : smoke_clouds) {
                auto center = cloud->get_center(t);
                if (center.has_value()) {
                    active_cloud_centers.push_back(center.value());
                }
            }
            
            if (active_cloud_centers.empty()) {
                continue;
            }
            
            // 调用协同判断函数
            Vector3d missile_pos = missile_->get_position(t);
            if (Geometry::check_collective_obscuration(
                    missile_pos, 
                    active_cloud_centers, 
                    target_key_points_)) {
                obscured_time_points.insert(static_cast<int>(std::round(t / time_step_)));
            }
        }
        
        double total_time = obscured_time_points.size() * time_step_;
        return -total_time; // 返回负值用于最小化
        
    } catch (const std::exception&) {
        return 0.0; // 无效策略，返回最差分数
    }
}

std::pair<VectorXd, double> ObscurationOptimizer::differential_evolution(
    const std::vector<Bounds>& bounds,
    const DESettings& settings)
{
    return DifferentialEvolution::optimize(
        [this](const VectorXd& x) { return this->objective_function(x); },
        bounds,
        settings
    );
}

// DifferentialEvolution Implementation
std::pair<VectorXd, double> DifferentialEvolution::optimize(
    ObjectiveFunction objective,
    const std::vector<Bounds>& bounds,
    const DESettings& settings)
{
    const int dim = bounds.size();
    std::mt19937 rng(std::random_device{}());
    
    // 初始化种群
    auto population = initialize_population(bounds, settings.population_size, rng);
    std::vector<double> fitness(settings.population_size);
    
    // 设置OpenMP线程数
    int num_threads = settings.num_threads;
    if (num_threads == -1) {
        num_threads = omp_get_max_threads();
    }
    omp_set_num_threads(num_threads);
    
    // 评估初始种群
    #pragma omp parallel for
    for (int i = 0; i < settings.population_size; ++i) {
        fitness[i] = objective(population[i]);
    }
    
    // 找到最佳个体
    auto best_it = std::min_element(fitness.begin(), fitness.end());
    int best_idx = std::distance(fitness.begin(), best_it);
    VectorXd best_individual = population[best_idx];
    double best_fitness = *best_it;
    
    if (settings.verbose) {
        std::cout << "DE初始化完成，种群大小: " << settings.population_size 
                  << ", 线程数: " << num_threads 
                  << ", 初始最佳适应度: " << -best_fitness << std::endl;
    }
    
    // 主进化循环
    for (int iteration = 0; iteration < settings.max_iterations; ++iteration) {
        std::vector<VectorXd> trial_population(settings.population_size);
        std::vector<double> trial_fitness(settings.population_size);
        
        // 生成试验向量
        #pragma omp parallel for
        for (int i = 0; i < settings.population_size; ++i) {
            std::mt19937 local_rng(rng() + i); // 每个线程独立的随机数生成器
            trial_population[i] = mutate_and_crossover(
                population, i, bounds, 
                settings.differential_weight, 
                settings.crossover_rate, 
                local_rng
            );
            trial_fitness[i] = objective(trial_population[i]);
        }
        
        // 选择操作
        bool improved = false;
        for (int i = 0; i < settings.population_size; ++i) {
            if (trial_fitness[i] < fitness[i]) {
                population[i] = trial_population[i];
                fitness[i] = trial_fitness[i];
                
                if (trial_fitness[i] < best_fitness) {
                    best_individual = trial_population[i];
                    best_fitness = trial_fitness[i];
                    improved = true;
                }
            }
        }
        
        // 输出进度
        if (settings.verbose && (iteration % 50 == 0 || improved)) {
            std::cout << "迭代 " << iteration << ", 最佳适应度: " << -best_fitness << std::endl;
        }
        
        // 收敛检查
        if (improved && std::abs(best_fitness) < settings.tolerance) {
            if (settings.verbose) {
                std::cout << "收敛于迭代 " << iteration << std::endl;
            }
            break;
        }
    }
    
    if (settings.verbose) {
        std::cout << "优化完成，最终适应度: " << -best_fitness << std::endl;
    }
    
    return {best_individual, best_fitness};
}

VectorXd DifferentialEvolution::generate_random_individual(
    const std::vector<Bounds>& bounds,
    std::mt19937& rng)
{
    VectorXd individual(bounds.size());
    
    for (size_t i = 0; i < bounds.size(); ++i) {
        std::uniform_real_distribution<double> dist(bounds[i].lower, bounds[i].upper);
        individual[i] = dist(rng);
    }
    
    return individual;
}

std::vector<VectorXd> DifferentialEvolution::initialize_population(
    const std::vector<Bounds>& bounds,
    int population_size,
    std::mt19937& rng)
{
    std::vector<VectorXd> population;
    population.reserve(population_size);
    
    for (int i = 0; i < population_size; ++i) {
        population.push_back(generate_random_individual(bounds, rng));
    }
    
    return population;
}

VectorXd DifferentialEvolution::mutate_and_crossover(
    const std::vector<VectorXd>& population,
    int target_idx,
    const std::vector<Bounds>& bounds,
    double differential_weight,
    double crossover_rate,
    std::mt19937& rng)
{
    const int population_size = population.size();
    const int dim = bounds.size();
    
    // 随机选择三个不同的个体 (排除目标个体)
    std::vector<int> candidates;
    for (int i = 0; i < population_size; ++i) {
        if (i != target_idx) {
            candidates.push_back(i);
        }
    }
    
    std::shuffle(candidates.begin(), candidates.end(), rng);
    
    int a = candidates[0];
    int b = candidates[1];
    int c = candidates[2];
    
    // 变异: V = X_a + F * (X_b - X_c)
    VectorXd mutant = population[a] + differential_weight * (population[b] - population[c]);
    ensure_bounds(mutant, bounds);
    
    // 交叉
    VectorXd trial = population[target_idx];
    std::uniform_real_distribution<double> uniform(0.0, 1.0);
    std::uniform_int_distribution<int> rand_dim(0, dim - 1);
    
    int random_dim = rand_dim(rng); // 确保至少有一个维度被交叉
    
    for (int i = 0; i < dim; ++i) {
        if (uniform(rng) < crossover_rate || i == random_dim) {
            trial[i] = mutant[i];
        }
    }
    
    ensure_bounds(trial, bounds);
    return trial;
}

void DifferentialEvolution::ensure_bounds(VectorXd& individual, const std::vector<Bounds>& bounds) {
    for (size_t i = 0; i < bounds.size(); ++i) {
        individual[i] = std::clamp(individual[i], bounds[i].lower, bounds[i].upper);
    }
}

} // namespace Optimizer
