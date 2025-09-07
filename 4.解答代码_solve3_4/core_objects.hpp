#ifndef CORE_OBJECTS_HPP
#define CORE_OBJECTS_HPP

#include <vector>
#include <array>
#include <string>
#include <memory>
#include "config.hpp"

/**
 * 目标圆柱体类
 */
class TargetCylinder {
public:
    TargetCylinder(const TargetSpecs& specs, int num_circ_samples = 16, int num_height_samples = 5);
    
    const std::vector<std::array<double, 3>>& get_key_points() const;
    
    double radius;
    double height;
    std::array<double, 3> bottom_center;
    std::array<double, 3> top_center;

private:
    std::vector<std::array<double, 3>> key_points_;
    
    void generate_full_key_points(int num_circ_samples, int num_height_samples);
};

/**
 * 导弹类
 */
class Missile {
public:
    explicit Missile(const std::string& missile_id);
    
    std::array<double, 3> get_position(double t) const;
    
    std::string id;
    std::array<double, 3> start_pos;
    double speed;
    std::array<double, 3> unit_vec;
};

/**
 * 烟雾云团类
 */
class SmokeCloud {
public:
    SmokeCloud(const std::array<double, 3>& detonate_pos, double detonate_time);
    
    bool get_center(double t, std::array<double, 3>& center) const;
    
    std::array<double, 3> detonate_pos;
    double start_time;
    double end_time;
};

/**
 * 烟雾弹类
 */
class Grenade {
public:
    Grenade(const std::array<double, 3>& deploy_pos, const std::array<double, 3>& deploy_vel, 
            double deploy_time, double fuse_time);
    
    std::unique_ptr<SmokeCloud> generate_smoke_cloud() const;
    
    double deploy_time;
    double fuse_time;
    double detonate_time;
    std::array<double, 3> detonate_pos;

private:
    std::array<double, 3> solve_trajectory_rk4(
        const std::array<double, 3>& deploy_pos, const std::array<double, 3>& deploy_vel
    );
};

/**
 * 无人机类
 */
class UAV {
public:
    explicit UAV(const std::string& uav_id);
    
    void set_flight_strategy(double speed, double angle);
    std::array<double, 3> get_position(double t) const;
    std::unique_ptr<Grenade> deploy_grenade(double deploy_time, double fuse_time) const;
    
    std::string id;
    std::array<double, 3> start_pos;
    double speed;
    double angle;
    std::array<double, 3> velocity_vec;
    bool strategy_set;
};

/**
 * 轨迹积分器类 - 用于高精度弹道计算
 */
class TrajectoryIntegrator {
public:
    static std::array<double, 3> integrate_trajectory_rk4(
        const std::array<double, 3>& initial_pos,
        const std::array<double, 3>& initial_vel,
        double time_duration,
        double dt = 0.01
    );

private:
    static std::array<double, 6> derivative_function(
        const std::array<double, 6>& state,
        double mass, double drag_factor
    );
};

#endif // CORE_OBJECTS_HPP
