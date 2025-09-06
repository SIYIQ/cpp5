#ifndef OPTIMIZER_HPP
#define OPTIMIZER_HPP

#include <vector>
#include <string>
#include <map>
#include <utility>
#include "core_objects.hpp"

// Forward declaration for nlopt's C-style struct
struct nlopt_opt_s;

// Structs to hold the parsed strategy for clarity
struct GrenadeStrategy {
    double t_deploy;
    double t_fuse;
    std::string target_missile;
};

struct UAVStrategy {
    double speed;
    double angle;
    std::vector<GrenadeStrategy> grenades;
};

using GlobalStrategy = std::map<std::string, UAVStrategy>;


class GlobalOptimizer {
public:
    GlobalOptimizer(
        const std::vector<std::string>& uav_ids,
        const std::vector<std::string>& missile_ids,
        const std::map<std::string, double>& threat_weights,
        const std::map<std::string, int>& uav_grenade_counts
    );

    // The main method to run the optimization
    std::pair<GlobalStrategy, double> solve(
        const std::vector<double>& lower_bounds,
        const std::vector<double>& upper_bounds,
        int population_size,
        int max_eval
    );

    // Public method to calculate details for a given strategy
    std::map<std::string, double> calculate_strategy_details(const GlobalStrategy& strategy);

    // Public static wrapper for the NLopt C-style callback
    static double objective_function_wrapper(unsigned n, const double *x, double *grad, void *data);

private:
    // The actual implementation of the objective function
    double objective_function_impl(unsigned n, const double *x);

    // Helper to parse the decision variable vector
    GlobalStrategy parse_decision_variables(const std::vector<double>& dv) const;

    // Member variables
    std::vector<std::string> uav_ids_;
    std::vector<std::string> missile_ids_;
    std::map<std::string, double> threat_weights_;
    std::map<std::string, int> uav_grenade_counts_;

    std::map<std::string, UAV> uavs_;
    std::map<std::string, Missile> missiles_;
    TargetCylinder target_;

    double time_step_;
    int num_missiles_;
};

#endif // OPTIMIZER_HPP
