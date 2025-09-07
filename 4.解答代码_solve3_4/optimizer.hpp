#ifndef OPTIMIZER_HPP
#define OPTIMIZER_HPP

#include <vector>
#include <unordered_map>
#include <string>
#include <memory>
#include <functional>
#include <random>
#include "core_objects.hpp"

// 差分进化算法的设置结构
struct DESettings {
    int popsize = 50;
    int maxiter = 1000;
    double F = 0.8;          // 差分权重因子
    double CR = 0.9;         // 交叉概率
    double tol = 0.01;       // 收敛容差
    bool disp = true;        // 是否显示进度
    int workers = 1;         // 并行工作线程数
    unsigned int seed = 42;  // 随机种子
};

// 优化变量的边界
using Bounds = std::vector<std::pair<double, double>>;

// UAV策略结构
struct GrenadeStrategy {
    double t_deploy;
    double t_fuse;
};

struct UAVStrategy {
    double speed;
    double angle;
    std::vector<GrenadeStrategy> grenades;
};

using StrategyMap = std::unordered_map<std::string, UAVStrategy>;

/**
 * 遮蔽优化器基类
 */
class ObscurationOptimizer {
public:
    ObscurationOptimizer(const std::string& missile_id, 
                        const std::unordered_map<std::string, int>& uav_assignments);
    
    virtual ~ObscurationOptimizer() = default;
    
    // 纯虚函数，由子类实现
    virtual StrategyMap parse_decision_variables(const std::vector<double>& dv) = 0;
    
    // 目标函数
    double objective_function(const std::vector<double>& decision_variables);
    
    // 求解方法
    std::pair<StrategyMap, double> solve(const Bounds& bounds, const DESettings& settings = DESettings());

protected:
    std::unique_ptr<Missile> missile_;
    std::unique_ptr<TargetCylinder> target_;
    std::unordered_map<std::string, int> uav_assignments_;
    double time_step_;
    std::vector<std::array<double, 3>> target_key_points_;

private:
    double evaluate_strategy(const StrategyMap& strategies);
};

/**
 * 差分进化算法实现
 */
class DifferentialEvolution {
public:
    using ObjectiveFunction = std::function<double(const std::vector<double>&)>;
    
    struct Result {
        std::vector<double> x;      // 最优解
        double fun;                 // 最优值
        int nit;                    // 迭代次数
        bool success;               // 是否成功收敛
    };
    
    static Result optimize(ObjectiveFunction func, const Bounds& bounds, const DESettings& settings);

private:
    static void ensure_bounds(std::vector<double>& individual, const Bounds& bounds);
    static std::vector<double> mutate(const std::vector<std::vector<double>>& population, 
                                    int target_idx, double F, std::mt19937& rng, const Bounds& bounds);
    static std::vector<double> crossover(const std::vector<double>& target, 
                                       const std::vector<double>& mutant, 
                                       double CR, std::mt19937& rng);
};

#endif // OPTIMIZER_HPP
