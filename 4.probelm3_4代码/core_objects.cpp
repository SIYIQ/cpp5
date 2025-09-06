#include "core_objects.hpp"
#define _USE_MATH_DEFINES
#include <cmath>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <limits>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// TargetCylinder 实现
TargetCylinder::TargetCylinder(const TargetSpecs& specs, int num_circ_samples, int num_height_samples) 
    : radius(specs.radius), height(specs.height) {
    bottom_center = specs.center_bottom;
    top_center = {bottom_center[0], bottom_center[1], bottom_center[2] + height};
    
    generate_full_key_points(num_circ_samples, num_height_samples);
}

const std::vector<std::array<double, 3>>& TargetCylinder::get_key_points() const {
    return key_points_;
}

void TargetCylinder::generate_full_key_points(int num_circ_samples, int num_height_samples) {
    key_points_.clear();
    
    // 1. 上下底面圆盘的采样 (圆周 + 圆心)
    key_points_.push_back(bottom_center);
    key_points_.push_back(top_center);
    
    for (int i = 0; i < num_circ_samples; ++i) {
        double angle = 2.0 * M_PI * i / num_circ_samples;
        std::array<double, 3> offset_xy = {
            radius * std::cos(angle), 
            radius * std::sin(angle), 
            0.0
        };
        
        // 底面圆周点
        key_points_.push_back({
            bottom_center[0] + offset_xy[0],
            bottom_center[1] + offset_xy[1],
            bottom_center[2] + offset_xy[2]
        });
        
        // 顶面圆周点
        key_points_.push_back({
            top_center[0] + offset_xy[0],
            top_center[1] + offset_xy[1],
            top_center[2] + offset_xy[2]
        });
    }
    
    // 2. 侧面母线的采样
    int num_side_samples = 4;
    for (int i = 0; i < num_side_samples; ++i) {
        double angle = 2.0 * M_PI * i / num_side_samples;
        std::array<double, 3> offset_xy = {
            radius * std::cos(angle), 
            radius * std::sin(angle), 
            0.0
        };
        
        // 在母线上从下到上均匀取点 (不含端点，因为已被圆周覆盖)
        for (int j = 1; j < num_height_samples; ++j) {
            double height_fraction = static_cast<double>(j) / num_height_samples;
            std::array<double, 3> height_offset = {0.0, 0.0, height * height_fraction};
            
            key_points_.push_back({
                bottom_center[0] + offset_xy[0] + height_offset[0],
                bottom_center[1] + offset_xy[1] + height_offset[1],
                bottom_center[2] + offset_xy[2] + height_offset[2]
            });
        }
    }
}

// Missile 实现
Missile::Missile(const std::string& missile_id) : id(missile_id) {
    auto it = MISSILES_INITIAL.find(missile_id);
    if (it == MISSILES_INITIAL.end()) {
        throw std::runtime_error("Unknown missile ID: " + missile_id);
    }
    
    const auto& specs = it->second;
    start_pos = specs.pos;
    speed = specs.speed;
    
    // 计算方向单位向量
    std::array<double, 3> direction_vec = {
        specs.target[0] - start_pos[0],
        specs.target[1] - start_pos[1],
        specs.target[2] - start_pos[2]
    };
    
    double norm = std::sqrt(direction_vec[0]*direction_vec[0] + 
                           direction_vec[1]*direction_vec[1] + 
                           direction_vec[2]*direction_vec[2]);
    
    unit_vec = {direction_vec[0]/norm, direction_vec[1]/norm, direction_vec[2]/norm};
}

std::array<double, 3> Missile::get_position(double t) const {
    return {
        start_pos[0] + unit_vec[0] * speed * t,
        start_pos[1] + unit_vec[1] * speed * t,
        start_pos[2] + unit_vec[2] * speed * t
    };
}

// SmokeCloud 实现
SmokeCloud::SmokeCloud(const std::array<double, 3>& detonate_pos, double detonate_time)
    : detonate_pos(detonate_pos), start_time(detonate_time), end_time(detonate_time + CLOUD_DURATION) {
}

bool SmokeCloud::get_center(double t, std::array<double, 3>& center) const {
    if (!(start_time <= t && t < end_time)) {
        return false;
    }
    
    double t_since_detonate = t - start_time;
    center = {
        detonate_pos[0],
        detonate_pos[1],
        detonate_pos[2] - CLOUD_SINK_SPEED * t_since_detonate
    };
    
    return true;
}

// TrajectoryIntegrator 实现
std::array<double, 6> TrajectoryIntegrator::derivative_function(
    const std::array<double, 6>& state,
    double mass, double drag_factor
) {
    // state = [x, y, z, vx, vy, vz]
    std::array<double, 3> velocity = {state[3], state[4], state[5]};
    
    // 计算加速度
    std::array<double, 3> gravity_accel = {0.0, 0.0, -G};
    
    double speed = std::sqrt(velocity[0]*velocity[0] + velocity[1]*velocity[1] + velocity[2]*velocity[2]);
    std::array<double, 3> drag_accel = {0.0, 0.0, 0.0};
    
    if (speed > 1e-6) {
        double drag_factor_normalized = drag_factor / mass;
        drag_accel = {
            -drag_factor_normalized * speed * velocity[0],
            -drag_factor_normalized * speed * velocity[1],
            -drag_factor_normalized * speed * velocity[2]
        };
    }
    
    std::array<double, 3> total_accel = {
        gravity_accel[0] + drag_accel[0],
        gravity_accel[1] + drag_accel[1],
        gravity_accel[2] + drag_accel[2]
    };
    
    // 返回状态向量的导数 [d(pos)/dt, d(vel)/dt] = [vel, accel]
    return {
        velocity[0], velocity[1], velocity[2],
        total_accel[0], total_accel[1], total_accel[2]
    };
}

std::array<double, 3> TrajectoryIntegrator::integrate_trajectory_rk4(
    const std::array<double, 3>& initial_pos,
    const std::array<double, 3>& initial_vel,
    double time_duration,
    double dt
) {
    // 初始状态向量 [x, y, z, vx, vy, vz]
    std::array<double, 6> state = {
        initial_pos[0], initial_pos[1], initial_pos[2],
        initial_vel[0], initial_vel[1], initial_vel[2]
    };
    
    double t = 0.0;
    while (t < time_duration) {
        double actual_dt = std::min(dt, time_duration - t);
        
        // RK4 方法
        auto k1 = derivative_function(state, GRENADE_MASS, GRENADE_DRAG_FACTOR);
        
        std::array<double, 6> state_k2;
        for (int i = 0; i < 6; ++i) {
            state_k2[i] = state[i] + 0.5 * actual_dt * k1[i];
        }
        auto k2 = derivative_function(state_k2, GRENADE_MASS, GRENADE_DRAG_FACTOR);
        
        std::array<double, 6> state_k3;
        for (int i = 0; i < 6; ++i) {
            state_k3[i] = state[i] + 0.5 * actual_dt * k2[i];
        }
        auto k3 = derivative_function(state_k3, GRENADE_MASS, GRENADE_DRAG_FACTOR);
        
        std::array<double, 6> state_k4;
        for (int i = 0; i < 6; ++i) {
            state_k4[i] = state[i] + actual_dt * k3[i];
        }
        auto k4 = derivative_function(state_k4, GRENADE_MASS, GRENADE_DRAG_FACTOR);
        
        // 更新状态
        for (int i = 0; i < 6; ++i) {
            state[i] += (actual_dt / 6.0) * (k1[i] + 2*k2[i] + 2*k3[i] + k4[i]);
        }
        
        t += actual_dt;
    }
    
    return {state[0], state[1], state[2]};
}

// Grenade 实现
Grenade::Grenade(const std::array<double, 3>& deploy_pos, const std::array<double, 3>& deploy_vel, 
                 double deploy_time, double fuse_time)
    : deploy_time(deploy_time), fuse_time(fuse_time), detonate_time(deploy_time + fuse_time) {
    
    detonate_pos = solve_trajectory_rk4(deploy_pos, deploy_vel);
}

std::array<double, 3> Grenade::solve_trajectory_rk4(
    const std::array<double, 3>& deploy_pos, const std::array<double, 3>& deploy_vel
) {
    return TrajectoryIntegrator::integrate_trajectory_rk4(deploy_pos, deploy_vel, fuse_time);
}

std::unique_ptr<SmokeCloud> Grenade::generate_smoke_cloud() const {
    return std::make_unique<SmokeCloud>(detonate_pos, detonate_time);
}

// UAV 实现
UAV::UAV(const std::string& uav_id) : id(uav_id), strategy_set(false) {
    auto it = UAVS_INITIAL.find(uav_id);
    if (it == UAVS_INITIAL.end()) {
        throw std::runtime_error("Unknown UAV ID: " + uav_id);
    }
    
    start_pos = it->second.pos;
}

void UAV::set_flight_strategy(double speed, double angle) {
    this->speed = speed;
    this->angle = angle;
    this->velocity_vec = {
        speed * std::cos(angle),
        speed * std::sin(angle),
        0.0
    };
    this->strategy_set = true;
}

std::array<double, 3> UAV::get_position(double t) const {
    if (!strategy_set) {
        throw std::runtime_error("UAV flight strategy has not been set.");
    }
    
    return {
        start_pos[0] + velocity_vec[0] * t,
        start_pos[1] + velocity_vec[1] * t,
        start_pos[2] + velocity_vec[2] * t
    };
}

std::unique_ptr<Grenade> UAV::deploy_grenade(double deploy_time, double fuse_time) const {
    if (!strategy_set) {
        throw std::runtime_error("UAV flight strategy has not been set.");
    }
    
    auto deploy_pos = get_position(deploy_time);
    return std::make_unique<Grenade>(deploy_pos, velocity_vec, deploy_time, fuse_time);
}
