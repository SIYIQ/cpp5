#include "core_objects.hpp"
#include <cmath>
#include <stdexcept>
#include <algorithm>
#include <vector>

namespace CoreObjects {

// TargetCylinder Implementation
TargetCylinder::TargetCylinder(const Config::TargetSpecs& specs, 
                               int num_circ_samples, 
                               int num_height_samples)
    : radius_(specs.radius)
    , height_(specs.height)
    , bottom_center_(specs.center_bottom)
    , top_center_(specs.center_bottom + Vector3d(0.0, 0.0, specs.height))
{
    generate_full_key_points(num_circ_samples, num_height_samples);
}

void TargetCylinder::generate_full_key_points(int num_circ_samples, int num_height_samples) {
    std::vector<Vector3d> points;
    
    // 1. 上下底面圆盘的采样 (圆周 + 圆心)
    points.push_back(bottom_center_);
    points.push_back(top_center_);
    
    for (int i = 0; i < num_circ_samples; ++i) {
        double angle = 2.0 * M_PI * i / num_circ_samples;
        Vector3d offset_xy(radius_ * std::cos(angle), radius_ * std::sin(angle), 0.0);
        
        // 底面圆周点
        points.push_back(bottom_center_ + offset_xy);
        // 顶面圆周点
        points.push_back(top_center_ + offset_xy);
    }
    
    // 2. 侧面母线的采样
    constexpr int num_side_samples = 4;
    for (int i = 0; i < num_side_samples; ++i) {
        double angle = 2.0 * M_PI * i / num_side_samples;
        Vector3d offset_xy(radius_ * std::cos(angle), radius_ * std::sin(angle), 0.0);
        
        // 在母线上从下到上均匀取点 (不含端点，因为已被圆周覆盖)
        for (int j = 1; j < num_height_samples; ++j) {
            double height_fraction = static_cast<double>(j) / num_height_samples;
            Vector3d height_offset(0.0, 0.0, height_ * height_fraction);
            points.push_back(bottom_center_ + offset_xy + height_offset);
        }
    }
    
    // 转换为Eigen矩阵格式 (3xN)
    key_points_.resize(3, points.size());
    for (size_t i = 0; i < points.size(); ++i) {
        key_points_.col(i) = points[i];
    }
}

// Missile Implementation
Missile::Missile(const std::string& missile_id) : id_(missile_id) {
    auto it = Config::MISSILES_INITIAL.find(missile_id);
    if (it == Config::MISSILES_INITIAL.end()) {
        throw std::runtime_error("Unknown missile ID: " + missile_id);
    }
    
    const auto& specs = it->second;
    start_pos_ = specs.pos;
    speed_ = specs.speed;
    
    Vector3d direction_vec = specs.target - start_pos_;
    unit_vec_ = direction_vec.normalized();
}

Vector3d Missile::get_position(double t) const {
    return start_pos_ + unit_vec_ * speed_ * t;
}

// SmokeCloud Implementation
SmokeCloud::SmokeCloud(const Vector3d& detonate_pos, double detonate_time)
    : detonate_pos_(detonate_pos)
    , start_time_(detonate_time)
    , end_time_(detonate_time + Config::CLOUD_DURATION)
{
}

std::optional<Vector3d> SmokeCloud::get_center(double t) const {
    if (t < start_time_ || t >= end_time_) {
        return std::nullopt;
    }
    
    double t_since_detonate = t - start_time_;
    Vector3d sink_offset(0.0, 0.0, -Config::CLOUD_SINK_SPEED * t_since_detonate);
    return detonate_pos_ + sink_offset;
}

// TrajectoryIntegrator Implementation
Vector3d TrajectoryIntegrator::solve_trajectory(
    const Vector3d& deploy_pos,
    const Vector3d& deploy_vel, 
    double fuse_time,
    double mass,
    double drag_factor
) {
    // 使用4阶Runge-Kutta方法求解ODE
    // 状态向量: [x, y, z, vx, vy, vz]
    Eigen::VectorXd y(6);
    y << deploy_pos[0], deploy_pos[1], deploy_pos[2], 
         deploy_vel[0], deploy_vel[1], deploy_vel[2];
    
    double t = 0.0;
    double dt = 0.01; // 时间步长
    
    while (t < fuse_time) {
        double h = std::min(dt, fuse_time - t);
        
        // RK4积分步骤
        Eigen::VectorXd k1(6), k2(6), k3(6), k4(6);
        Eigen::VectorXd y_temp(6);
        
        grenade_motion_ode(t, y, k1, mass, drag_factor);
        
        y_temp = y + 0.5 * h * k1;
        grenade_motion_ode(t + 0.5*h, y_temp, k2, mass, drag_factor);
        
        y_temp = y + 0.5 * h * k2;
        grenade_motion_ode(t + 0.5*h, y_temp, k3, mass, drag_factor);
        
        y_temp = y + h * k3;
        grenade_motion_ode(t + h, y_temp, k4, mass, drag_factor);
        
        y += h/6.0 * (k1 + 2*k2 + 2*k3 + k4);
        t += h;
    }
    
    return Vector3d(y[0], y[1], y[2]);
}

void TrajectoryIntegrator::grenade_motion_ode(
    double t,
    const Eigen::VectorXd& y,
    Eigen::VectorXd& dydt,
    double mass,
    double drag_factor
) {
    // y[0:3] 是位置, y[3:6] 是速度
    Vector3d velocity(y[3], y[4], y[5]);
    
    // 计算加速度
    Vector3d gravity_accel(0.0, 0.0, -Config::G);
    
    double speed = velocity.norm();
    Vector3d drag_accel = Vector3d::Zero();
    if (speed > 1e-6) {
        drag_accel = -(drag_factor / mass) * speed * velocity;
    }
    
    Vector3d total_accel = gravity_accel + drag_accel;
    
    // 返回状态向量的导数 [d(pos)/dt, d(vel)/dt] = [vel, accel]
    dydt << velocity[0], velocity[1], velocity[2],
            total_accel[0], total_accel[1], total_accel[2];
}

// Grenade Implementation
Grenade::Grenade(const Vector3d& deploy_pos, 
                 const Vector3d& deploy_vel,
                 double deploy_time, 
                 double fuse_time)
    : deploy_time_(deploy_time)
    , fuse_time_(fuse_time)
    , detonate_time_(deploy_time + fuse_time)
{
    detonate_pos_ = TrajectoryIntegrator::solve_trajectory(deploy_pos, deploy_vel, fuse_time);
}

std::unique_ptr<SmokeCloud> Grenade::generate_smoke_cloud() const {
    return std::make_unique<SmokeCloud>(detonate_pos_, detonate_time_);
}

// UAV Implementation
UAV::UAV(const std::string& uav_id) 
    : id_(uav_id), strategy_set_(false) 
{
    auto it = Config::UAVS_INITIAL.find(uav_id);
    if (it == Config::UAVS_INITIAL.end()) {
        throw std::runtime_error("Unknown UAV ID: " + uav_id);
    }
    
    start_pos_ = it->second.pos;
}

void UAV::set_flight_strategy(double speed, double angle) {
    speed_ = speed;
    angle_ = angle;
    velocity_vec_ = speed * Vector3d(std::cos(angle), std::sin(angle), 0.0);
    strategy_set_ = true;
}

Vector3d UAV::get_position(double t) const {
    if (!strategy_set_) {
        throw std::runtime_error("UAV flight strategy has not been set.");
    }
    return start_pos_ + velocity_vec_ * t;
}

std::unique_ptr<Grenade> UAV::deploy_grenade(double deploy_time, double fuse_time) const {
    if (!strategy_set_) {
        throw std::runtime_error("UAV flight strategy has not been set.");
    }
    
    Vector3d deploy_pos = get_position(deploy_time);
    return std::make_unique<Grenade>(deploy_pos, velocity_vec_, deploy_time, fuse_time);
}

} // namespace CoreObjects
