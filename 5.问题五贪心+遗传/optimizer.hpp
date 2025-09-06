#pragma once

#include <vector>
#include <unordered_map>
#include <string>
#include <memory>
#include <functional>
#include <random>
#include <future>
#include <Eigen/Dense>
#include "config.hpp"
#include "core_objects.hpp"
#include "geometry.hpp"

using Vector3d = Eigen::Vector3d;
using VectorXd = Eigen::VectorXd;

namespace Optimizer {

/**
 * @brief 优化边界结构
 */
struct Bounds {
    double lower;
    double upper;
    
    Bounds(double l, double u) : lower(l), upper(u) {}
};

/**
 * @brief 无人机策略结构
 */
struct UAVStrategy {
    double speed;
    double angle;
    
    struct GrenadeDeployment {
        double t_deploy;
        double t_fuse;
        std::string target_missile;  // 新增：目标导弹ID
    };
    
    std::vector<GrenadeDeployment> grenades;
};

using StrategyMap = std::unordered_map<std::string, UAVStrategy>;

/**
 * @brief 差分进化优化器设置
 */
struct DESettings {
    int population_size = 150;
    int max_iterations = 1000;
    double tolerance = 1e-6;
    double crossover_rate = 0.7;
    double differential_weight = 0.8;
    int num_threads = -1; // -1表示使用所有可用线程
    bool verbose = true;
    
    DESettings() = default;
};

/**
 * @brief 抽象遮蔽优化器基类
 */
class ObscurationOptimizer {
public:
    ObscurationOptimizer(const std::string& missile_id, 
                        const std::unordered_map<std::string, int>& uav_assignments);
    
    virtual ~ObscurationOptimizer() = default;
    
    /**
     * @brief 求解优化问题
     */
    std::pair<StrategyMap, double> solve(const std::vector<Bounds>& bounds, 
                                        const DESettings& settings = DESettings());

protected:
    /**
     * @brief 纯虚函数：解析决策变量 (由子类实现)
     */
    virtual StrategyMap parse_decision_variables(const VectorXd& decision_variables) = 0;
    
    /**
     * @brief 目标函数：计算遮蔽时间 (返回负值用于最小化)
     */
    double objective_function(const VectorXd& decision_variables);

protected:
    std::unique_ptr<CoreObjects::Missile> missile_;
    std::unique_ptr<CoreObjects::TargetCylinder> target_;
    std::unordered_map<std::string, int> uav_assignments_;
    double time_step_;
    Eigen::Matrix3Xd target_key_points_;

private:
    
    // 差分进化算法实现
    std::pair<VectorXd, double> differential_evolution(
        const std::vector<Bounds>& bounds,
        const DESettings& settings
    );
    
    // 并行目标函数评估
    std::vector<double> evaluate_population_parallel(
        const std::vector<VectorXd>& population,
        int num_threads
    );
    
    // 随机数生成器
    mutable std::mt19937 rng_;
};

/**
 * @brief 高性能差分进化算法实现
 */
class DifferentialEvolution {
public:
    using ObjectiveFunction = std::function<double(const VectorXd&)>;
    
    static std::pair<VectorXd, double> optimize(
        ObjectiveFunction objective,
        const std::vector<Bounds>& bounds,
        const DESettings& settings = DESettings()
    );

private:
    static VectorXd generate_random_individual(
        const std::vector<Bounds>& bounds,
        std::mt19937& rng
    );
    
    static std::vector<VectorXd> initialize_population(
        const std::vector<Bounds>& bounds,
        int population_size,
        std::mt19937& rng
    );
    
    static VectorXd mutate_and_crossover(
        const std::vector<VectorXd>& population,
        int target_idx,
        const std::vector<Bounds>& bounds,
        double differential_weight,
        double crossover_rate,
        std::mt19937& rng
    );
    
    static void ensure_bounds(VectorXd& individual, const std::vector<Bounds>& bounds);
};

/**
 * @brief 全局协同优化器 - 所有导弹考虑场上所有烟雾云
 */
class GlobalOptimizer {
public:
    GlobalOptimizer(const std::vector<std::string>& uav_ids,
                    const std::vector<std::string>& missile_ids,
                    const std::unordered_map<std::string, double>& threat_weights,
                    const std::unordered_map<std::string, int>& uav_grenade_counts);
    
    std::pair<StrategyMap, double> solve(const std::vector<Bounds>& bounds, 
                                         const DESettings& settings = DESettings());
    
    std::unordered_map<std::string, double> calculate_strategy_details(const StrategyMap& strategy);

private:
    StrategyMap parse_decision_variables(const VectorXd& decision_variables);
    double objective_function_impl(const VectorXd& decision_variables);
    
    std::vector<std::string> uav_ids_;
    std::vector<std::string> missile_ids_;
    std::unordered_map<std::string, double> threat_weights_;
    std::unordered_map<std::string, int> uav_grenade_counts_;
    
    std::unordered_map<std::string, CoreObjects::UAV> uavs_;
    std::unordered_map<std::string, CoreObjects::Missile> missiles_;
    CoreObjects::TargetCylinder target_;
    Eigen::Matrix3Xd target_key_points_;
    double time_step_;
    int num_missiles_;
};

} // namespace Optimizer
