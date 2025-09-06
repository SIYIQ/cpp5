#include "optimizer.hpp"
#include "geometry.hpp"
#include <nlopt.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include <numeric>
#include <algorithm>

GlobalOptimizer::GlobalOptimizer(
    const std::vector<std::string>& uav_ids,
    const std::vector<std::string>& missile_ids,
    const std::map<std::string, double>& threat_weights,
    const std::map<std::string, int>& uav_grenade_counts)
    : uav_ids_(uav_ids),
      missile_ids_(missile_ids),
      threat_weights_(threat_weights),
      uav_grenade_counts_(uav_grenade_counts),
      target_(TRUE_TARGET_SPECS),
      time_step_(0.1),
      num_missiles_(missile_ids.size())
{
    for (const auto& id : uav_ids_) {
        uavs_.emplace(id, UAV(id));
    }
    for (const auto& id : missile_ids_) {
        missiles_.emplace(id, Missile(id));
    }
}

// Static wrapper function to be passed to NLopt
double GlobalOptimizer::objective_function_wrapper(unsigned n, const double *x, double *grad, void *data) {
    // Cast the user data back to a pointer to the GlobalOptimizer instance
    GlobalOptimizer* optimizer = static_cast<GlobalOptimizer*>(data);
    // Call the actual implementation
    return optimizer->objective_function_impl(n, x);
}

// Implementation of the objective function logic
double GlobalOptimizer::objective_function_impl(unsigned n, const double *x) {
    std::vector<double> dv(x, x + n);
    GlobalStrategy strategy;
    try {
        strategy = parse_decision_variables(dv);
    } catch (const std::out_of_range& e) {
        // Invalid index access during parsing
        return 1e9; // Return a large penalty
    }

    std::vector<SmokeCloud> all_smoke_clouds;
    double min_start_time = 1e9;
    double max_end_time = 0.0;

    for (const auto& [uav_id, uav_strat] : strategy) {
        auto& uav = uavs_.at(uav_id);
        uav.set_flight_strategy(uav_strat.speed, uav_strat.angle);
        for (const auto& g_strat : uav_strat.grenades) {
            Grenade grenade = uav.deploy_grenade(g_strat.t_deploy, g_strat.t_fuse);
            SmokeCloud cloud = grenade.generate_smoke_cloud();
            cloud.target_missile_id = g_strat.target_missile;
            all_smoke_clouds.push_back(cloud);
            if (cloud.start_time < min_start_time) min_start_time = cloud.start_time;
            if (cloud.end_time > max_end_time) max_end_time = cloud.end_time;
        }
    }

    if (all_smoke_clouds.empty()) {
        return 1e9;
    }

    std::map<std::string, double> total_obscured_time_per_missile;
    for(const auto& id : missile_ids_) {
        total_obscured_time_per_missile[id] = 0.0;
    }

    for (double t = min_start_time; t < max_end_time; t += time_step_) {
        for (const auto& missile_id : missile_ids_) {
            std::vector<Eigen::Vector3d> active_cloud_centers_for_missile;
            for (const auto& cloud : all_smoke_clouds) {
                if (cloud.target_missile_id == missile_id) {
                    if (auto center = cloud.get_center(t)) {
                        active_cloud_centers_for_missile.push_back(*center);
                    }
                }
            }

            if (active_cloud_centers_for_missile.empty()) {
                continue;
            }

            Eigen::Vector3d missile_pos = missiles_.at(missile_id).get_position(t);
            if (check_collective_obscuration(missile_pos, active_cloud_centers_for_missile, target_.get_key_points())) {
                total_obscured_time_per_missile[missile_id] += time_step_;
            }
        }
    }

    double total_weighted_score = 0.0;
    for (const auto& [missile_id, obs_time] : total_obscured_time_per_missile) {
        total_weighted_score += threat_weights_.at(missile_id) * obs_time;
    }

    return -total_weighted_score; // NLopt minimizes, so we return the negative score
}

GlobalStrategy GlobalOptimizer::parse_decision_variables(const std::vector<double>& dv) const {
    GlobalStrategy strategy;
    int dv_index = 0;

    for (const auto& uav_id : uav_ids_) {
        int num_grenades = uav_grenade_counts_.at(uav_id);
        UAVStrategy uav_strat;
        uav_strat.speed = dv.at(dv_index++);
        uav_strat.angle = dv.at(dv_index++);

        double last_td = 0;
        for (int i = 0; i < num_grenades; ++i) {
            GrenadeStrategy g_strat;
            double t_d_or_delta = dv.at(dv_index++);
            g_strat.t_fuse = dv.at(dv_index++);
            double target_selector = dv.at(dv_index++);

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

std::pair<GlobalStrategy, double> GlobalOptimizer::solve(
    const std::vector<double>& lower_bounds,
    const std::vector<double>& upper_bounds,
    int population_size,
    int max_eval) 
{
    unsigned int dim = lower_bounds.size();
    nlopt::opt opt(nlopt::GN_ESCH, dim); // Using Evolutionary Strategy

    opt.set_lower_bounds(lower_bounds);
    opt.set_upper_bounds(upper_bounds);

    opt.set_min_objective(GlobalOptimizer::objective_function_wrapper, this);
    
    // ESCH does not use a local optimizer, so the related code is removed.

    opt.set_population(population_size);
    opt.set_maxeval(max_eval);

    std::vector<double> x = lower_bounds; // Initial guess
    double min_f;

    std::cout << "Starting NLopt optimization..." << std::endl;
    nlopt::result result = opt.optimize(x, min_f);
    std::cout << "NLopt optimization finished." << std::endl;

    GlobalStrategy best_strategy = parse_decision_variables(x);
    double max_score = -min_f;

    return {best_strategy, max_score};
}
