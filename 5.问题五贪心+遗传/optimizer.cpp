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

// GlobalOptimizer Implementation
GlobalOptimizer::GlobalOptimizer(const std::vector<std::string>& uav_ids,
                                 const std::vector<std::string>& missile_ids,
                                 const std::unordered_map<std::string, double>& threat_weights,
                                 const std::unordered_map<std::string, int>& uav_grenade_counts)
    : uav_ids_(uav_ids)
    , missile_ids_(missile_ids)
    , threat_weights_(threat_weights)
    , uav_grenade_counts_(uav_grenade_counts)
    , target_(Config::TRUE_TARGET_SPECS)
    , time_step_(0.1)
    , num_missiles_(missile_ids.size())
{
    for (const auto& id : uav_ids_) {
        uavs_.emplace(id, CoreObjects::UAV(id));
    }
    for (const auto& id : missile_ids_) {
        missiles_.emplace(id, CoreObjects::Missile(id));
    }
    target_key_points_ = target_.get_key_points();
}

std::pair<StrategyMap, double> GlobalOptimizer::solve(const std::vector<Bounds>& bounds, 
                                                      const DESettings& settings) {
    
    ObjectiveFunction obj_func = [this](const VectorXd& dv) -> double {
        return this->objective_function_impl(dv);
    };
    
    auto [optimal_vars, min_score] = DifferentialEvolution::optimize(obj_func, bounds, settings);
    double max_score = -min_score;
    
    StrategyMap optimal_strategy = parse_decision_variables(optimal_vars);
    
    return {optimal_strategy, max_score};
}

StrategyMap GlobalOptimizer::parse_decision_variables(const VectorXd& decision_variables) {
    StrategyMap strategy;
    int dv_index = 0;
    
    for (const auto& uav_id : uav_ids_) {
        int num_grenades = uav_grenade_counts_.at(uav_id);
        UAVStrategy uav_strat;
        
        uav_strat.speed = decision_variables[dv_index++];
        uav_strat.angle = decision_variables[dv_index++];
        
        double last_td = 0.0;
        for (int i = 0; i < num_grenades; ++i) {
            UAVStrategy::GrenadeDeployment g_strat;
            
            double t_d_or_delta = decision_variables[dv_index++];
            g_strat.t_fuse = decision_variables[dv_index++];
            double target_selector = decision_variables[dv_index++];
            
            g_strat.t_deploy = (i == 0) ? t_d_or_delta : last_td + t_d_or_delta;
            last_td = g_strat.t_deploy;
            
            int target_missile_index = std::min(static_cast<int>(target_selector * num_missiles_), num_missiles_ - 1);
            g_strat.target_missile = missile_ids_.at(target_missile_index);
            
            uav_strat.grenades.push_back(g_strat);
        }
        strategy[uav_id] = uav_strat;
    }
    return strategy;
}

double GlobalOptimizer::objective_function_impl(const VectorXd& decision_variables) {
    StrategyMap strategy;
    try {
        strategy = parse_decision_variables(decision_variables);
    } catch (const std::exception& e) {
        return 1e9;
    }

    std::vector<std::unique_ptr<CoreObjects::SmokeCloud>> all_smoke_clouds;
    
    // 按无人机生成所有弹药并转化为烟云
    for (const auto& [uav_id, uav_strat] : strategy) {
        auto& uav = uavs_.at(uav_id);
        uav.set_flight_strategy(uav_strat.speed, uav_strat.angle);
        for (const auto& g_strat : uav_strat.grenades) {
            auto grenade = uav.deploy_grenade(g_strat.t_deploy, g_strat.t_fuse);
            // 注意：这里不再将烟云与特定导弹绑定
            all_smoke_clouds.push_back(grenade->generate_smoke_cloud());
        }
    }

    if (all_smoke_clouds.empty()) {
        return 1e9;
    }

    // 确定联合模拟的起止时间
    double sim_start_time = all_smoke_clouds[0]->get_start_time();
    double sim_end_time = all_smoke_clouds[0]->get_end_time();
    for (const auto& cloud : all_smoke_clouds) {
        sim_start_time = std::min(sim_start_time, cloud->get_start_time());
        sim_end_time = std::max(sim_end_time, cloud->get_end_time());
    }

    // 为每个导弹计算其被遮蔽的时间（考虑场上所有烟雾云）
    std::unordered_map<std::string, double> total_obscured_time_per_missile;
    for (const auto& missile_id : missile_ids_) {
        total_obscured_time_per_missile[missile_id] = 0.0;
    }

    for (double t = sim_start_time; t < sim_end_time; t += time_step_) {
        // 获取当前时刻场上所有有效的烟云中心
        std::vector<Eigen::Vector3d> active_cloud_centers;
        for (const auto& cloud : all_smoke_clouds) {
            auto center = cloud->get_center(t);
            if (center) {
                active_cloud_centers.push_back(*center);
            }
        }
        
        if (active_cloud_centers.empty()) {
            continue;
        }

        // 对每个导弹独立判断是否被场上所有烟云协同遮蔽
        for (const auto& missile_id : missile_ids_) {
            const auto& missile = missiles_.at(missile_id);
            Eigen::Vector3d missile_pos = missile.get_position(t);
            
            if (Geometry::check_collective_obscuration(missile_pos, active_cloud_centers, target_key_points_)) {
                total_obscured_time_per_missile[missile_id] += time_step_;
            }
        }
    }
    
    // 计算加权总分
    double total_weighted_score = 0.0;
    for (const auto& [missile_id, obs_time] : total_obscured_time_per_missile) {
        total_weighted_score += threat_weights_.at(missile_id) * obs_time;
    }
    
    // 优化器是最小化，所以返回分数的负数
    return -total_weighted_score;
}

std::unordered_map<std::string, double> GlobalOptimizer::calculate_strategy_details(const StrategyMap& strategy) {
    std::unordered_map<std::string, double> total_obscured_time_per_missile;
    for (const auto& id : missile_ids_) {
        total_obscured_time_per_missile[id] = 0.0;
    }

    std::vector<std::unique_ptr<CoreObjects::SmokeCloud>> all_smoke_clouds;
    double min_start_time = 1e9;
    double max_end_time = 0.0;

    for (const auto& [uav_id, uav_strat] : strategy) {
        auto& uav = uavs_.at(uav_id);
        uav.set_flight_strategy(uav_strat.speed, uav_strat.angle);
        for (const auto& g_strat : uav_strat.grenades) {
            auto grenade = uav.deploy_grenade(g_strat.t_deploy, g_strat.t_fuse);
            auto cloud = grenade->generate_smoke_cloud();
            all_smoke_clouds.push_back(std::move(cloud));
            if (all_smoke_clouds.back()->get_start_time() < min_start_time) min_start_time = all_smoke_clouds.back()->get_start_time();
            if (all_smoke_clouds.back()->get_end_time() > max_end_time) max_end_time = all_smoke_clouds.back()->get_end_time();
        }
    }

    if (all_smoke_clouds.empty()) {
        return total_obscured_time_per_missile;
    }

    for (double t = min_start_time; t < max_end_time; t += time_step_) {
        std::vector<Eigen::Vector3d> active_cloud_centers;
        for (const auto& cloud : all_smoke_clouds) {
            auto center = cloud->get_center(t);
            if (center) {
                active_cloud_centers.push_back(*center);
            }
        }

        if (active_cloud_centers.empty()) continue;

        for (const auto& missile_id : missile_ids_) {
            Eigen::Vector3d missile_pos = missiles_.at(missile_id).get_position(t);
            if (Geometry::check_collective_obscuration(missile_pos, active_cloud_centers, target_key_points_)) {
                total_obscured_time_per_missile[missile_id] += time_step_;
            }
        }
    }
    return total_obscured_time_per_missile;
}

} // namespace Optimizer
