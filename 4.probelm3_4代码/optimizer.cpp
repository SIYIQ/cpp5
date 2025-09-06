#include "optimizer.hpp"
#include "geometry.hpp"
#define _USE_MATH_DEFINES
#include <iostream>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <set>
#include <limits>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ObscurationOptimizer 实现
ObscurationOptimizer::ObscurationOptimizer(const std::string& missile_id, 
                                         const std::unordered_map<std::string, int>& uav_assignments)
    : uav_assignments_(uav_assignments), time_step_(0.1) {
    
    missile_ = std::make_unique<Missile>(missile_id);
    target_ = std::make_unique<TargetCylinder>(TRUE_TARGET_SPECS);
    
    // 预先获取关键点，避免在循环中重复调用
    target_key_points_ = target_->get_key_points();
}

double ObscurationOptimizer::objective_function(const std::vector<double>& decision_variables) {
    try {
        StrategyMap strategies = parse_decision_variables(decision_variables);
        return -evaluate_strategy(strategies); // 负号因为我们要最大化遮蔽时间
    } catch (const std::exception&) {
        return 0.0; // 无效策略，返回最差得分
    }
}

double ObscurationOptimizer::evaluate_strategy(const StrategyMap& strategies) {
    std::vector<std::unique_ptr<SmokeCloud>> smoke_clouds;
    
    try {
        for (const auto& [uav_id, uav_strat] : strategies) {
            UAV uav(uav_id);
            uav.set_flight_strategy(uav_strat.speed, uav_strat.angle);
            
            for (const auto& g_strat : uav_strat.grenades) {
                auto grenade = uav.deploy_grenade(g_strat.t_deploy, g_strat.t_fuse);
                smoke_clouds.push_back(grenade->generate_smoke_cloud());
            }
        }
    } catch (const std::exception&) {
        return 0.0; // 无效策略
    }

    if (smoke_clouds.empty()) {
        return 0.0;
    }

    // 计算仿真时间范围
    double sim_start_time = std::numeric_limits<double>::max();
    double sim_end_time = std::numeric_limits<double>::lowest();
    
    for (const auto& cloud : smoke_clouds) {
        sim_start_time = std::min(sim_start_time, cloud->start_time);
        sim_end_time = std::max(sim_end_time, cloud->end_time);
    }

    std::set<int> obscured_time_points;
    
    for (double t = sim_start_time; t < sim_end_time; t += time_step_) {
        // 获取当前所有有效的云团中心
        std::vector<std::array<double, 3>> active_cloud_centers;
        
        for (const auto& cloud : smoke_clouds) {
            std::array<double, 3> center;
            if (cloud->get_center(t, center)) {
                active_cloud_centers.push_back(center);
            }
        }
        
        if (active_cloud_centers.empty()) {
            continue;
        }

        // 调用协同遮蔽判断函数
        auto missile_pos = missile_->get_position(t);
        if (check_collective_obscuration(missile_pos, active_cloud_centers, target_key_points_)) {
            obscured_time_points.insert(static_cast<int>(std::round(t / time_step_)));
        }
    }
    
    return static_cast<double>(obscured_time_points.size()) * time_step_;
}

std::pair<StrategyMap, double> ObscurationOptimizer::solve(const Bounds& bounds, const DESettings& settings) {
    auto result = DifferentialEvolution::optimize(
        [this](const std::vector<double>& dv) { return this->objective_function(dv); },
        bounds,
        settings
    );
    
    double max_time = -result.fun;
    StrategyMap optimal_strategy = parse_decision_variables(result.x);
    
    return {optimal_strategy, max_time};
}

// DifferentialEvolution 实现
DifferentialEvolution::Result DifferentialEvolution::optimize(
    ObjectiveFunction func, const Bounds& bounds, const DESettings& settings
) {
    const int dim = static_cast<int>(bounds.size());
    const int popsize = (settings.popsize > 0) ? settings.popsize : 15 * dim;
    
    std::mt19937 rng(settings.seed);
    std::uniform_real_distribution<double> uniform(0.0, 1.0);
    
    // 初始化种群
    std::vector<std::vector<double>> population(popsize, std::vector<double>(dim));
    std::vector<double> fitness(popsize);
    
    for (int i = 0; i < popsize; ++i) {
        for (int j = 0; j < dim; ++j) {
            population[i][j] = bounds[j].first + uniform(rng) * (bounds[j].second - bounds[j].first);
        }
        fitness[i] = func(population[i]);
    }
    
    // 找到初始最优个体
    int best_idx = std::distance(fitness.begin(), std::min_element(fitness.begin(), fitness.end()));
    std::vector<double> best_x = population[best_idx];
    double best_f = fitness[best_idx];
    
    if (settings.disp) {
        std::cout << "差分进化算法开始优化..." << std::endl;
        std::cout << "初始最优值: " << std::fixed << std::setprecision(6) << best_f << std::endl;
    }
    
    // 主优化循环
    for (int generation = 0; generation < settings.maxiter; ++generation) {
        for (int i = 0; i < popsize; ++i) {
            // 变异
            auto mutant = mutate(population, i, settings.F, rng, bounds);
            
            // 交叉
            auto trial = crossover(population[i], mutant, settings.CR, rng);
            
            // 选择
            double trial_fitness = func(trial);
            if (trial_fitness < fitness[i]) {
                population[i] = trial;
                fitness[i] = trial_fitness;
                
                // 更新全局最优
                if (trial_fitness < best_f) {
                    best_x = trial;
                    best_f = trial_fitness;
                }
            }
        }
        
        // 显示进度
        if (settings.disp && (generation + 1) % 50 == 0) {
            std::cout << "第 " << std::setw(4) << (generation + 1) 
                      << " 代，当前最优值: " << std::fixed << std::setprecision(6) << best_f << std::endl;
        }
        
        // 检查收敛
        if (generation > 100) {
            double variance = 0.0;
            double mean = 0.0;
            for (double f : fitness) {
                mean += f;
            }
            mean /= popsize;
            
            for (double f : fitness) {
                variance += (f - mean) * (f - mean);
            }
            variance /= popsize;
            
            if (std::sqrt(variance) < settings.tol) {
                if (settings.disp) {
                    std::cout << "算法在第 " << (generation + 1) << " 代收敛。" << std::endl;
                }
                break;
            }
        }
    }
    
    if (settings.disp) {
        std::cout << "优化完成，最终最优值: " << std::fixed << std::setprecision(6) << best_f << std::endl;
    }
    
    return {best_x, best_f, settings.maxiter, true};
}

void DifferentialEvolution::ensure_bounds(std::vector<double>& individual, const Bounds& bounds) {
    for (size_t i = 0; i < individual.size(); ++i) {
        individual[i] = std::max(bounds[i].first, std::min(bounds[i].second, individual[i]));
    }
}

std::vector<double> DifferentialEvolution::mutate(const std::vector<std::vector<double>>& population, 
                                                int target_idx, double F, std::mt19937& rng, 
                                                const Bounds& bounds) {
    const int popsize = static_cast<int>(population.size());
    const int dim = static_cast<int>(population[0].size());
    
    std::uniform_int_distribution<int> int_dist(0, popsize - 1);
    
    // 随机选择三个不同的个体（且不等于target_idx）
    int r1, r2, r3;
    do { r1 = int_dist(rng); } while (r1 == target_idx);
    do { r2 = int_dist(rng); } while (r2 == target_idx || r2 == r1);
    do { r3 = int_dist(rng); } while (r3 == target_idx || r3 == r1 || r3 == r2);
    
    // DE/rand/1 策略: mutant = r1 + F * (r2 - r3)
    std::vector<double> mutant(dim);
    for (int i = 0; i < dim; ++i) {
        mutant[i] = population[r1][i] + F * (population[r2][i] - population[r3][i]);
    }
    
    ensure_bounds(mutant, bounds);
    return mutant;
}

std::vector<double> DifferentialEvolution::crossover(const std::vector<double>& target, 
                                                   const std::vector<double>& mutant, 
                                                   double CR, std::mt19937& rng) {
    const int dim = static_cast<int>(target.size());
    std::uniform_real_distribution<double> uniform(0.0, 1.0);
    std::uniform_int_distribution<int> int_dist(0, dim - 1);
    
    std::vector<double> trial = target;
    int j_rand = int_dist(rng); // 确保至少有一个基因来自mutant
    
    for (int i = 0; i < dim; ++i) {
        if (uniform(rng) < CR || i == j_rand) {
            trial[i] = mutant[i];
        }
    }
    
    return trial;
}
