#pragma once

#include <vector>
#include <string>
#include <memory>
#include <optional>
#include <Eigen/Dense>
#include "config.hpp"

using Vector3d = Eigen::Vector3d;
using Matrix3Xd = Eigen::Matrix3Xd;

namespace CoreObjects {

/**
 * @brief 目标圆柱体类
 */
class TargetCylinder {
public:
    TargetCylinder(const Config::TargetSpecs& specs, 
                   int num_circ_samples = 16, 
                   int num_height_samples = 5);
    
    const Matrix3Xd& get_key_points() const { return key_points_; }
    
    double get_radius() const { return radius_; }
    double get_height() const { return height_; }
    const Vector3d& get_bottom_center() const { return bottom_center_; }
    const Vector3d& get_top_center() const { return top_center_; }

private:
    double radius_;
    double height_;
    Vector3d bottom_center_;
    Vector3d top_center_;
    Matrix3Xd key_points_; // 3xN矩阵，每列是一个关键点
    
    void generate_full_key_points(int num_circ_samples, int num_height_samples);
};

/**
 * @brief 导弹类
 */
class Missile {
public:
    explicit Missile(const std::string& missile_id);
    
    Vector3d get_position(double t) const;
    
    const std::string& get_id() const { return id_; }
    const Vector3d& get_start_pos() const { return start_pos_; }
    double get_speed() const { return speed_; }

private:
    std::string id_;
    Vector3d start_pos_;
    double speed_;
    Vector3d unit_vec_; // 单位方向向量
};

/**
 * @brief 烟雾云类
 */
class SmokeCloud {
public:
    SmokeCloud(const Vector3d& detonate_pos, double detonate_time);
    
    std::optional<Vector3d> get_center(double t) const;
    
    double get_start_time() const { return start_time_; }
    double get_end_time() const { return end_time_; }

private:
    Vector3d detonate_pos_;
    double start_time_;
    double end_time_;
};

/**
 * @brief ODE求解器用于计算烟雾弹轨迹
 */
class TrajectoryIntegrator {
public:
    /**
     * @brief 计算烟雾弹从投放到起爆的轨迹终点
     */
    static Vector3d solve_trajectory(
        const Vector3d& deploy_pos,
        const Vector3d& deploy_vel, 
        double fuse_time,
        double mass = Config::GRENADE_MASS,
        double drag_factor = Config::GRENADE_DRAG_FACTOR
    );

private:
    /**
     * @brief 烟雾弹运动微分方程
     */
    static void grenade_motion_ode(
        double t,
        const Eigen::VectorXd& y,
        Eigen::VectorXd& dydt,
        double mass,
        double drag_factor
    );
};

/**
 * @brief 烟雾弹类
 */
class Grenade {
public:
    Grenade(const Vector3d& deploy_pos, 
            const Vector3d& deploy_vel,
            double deploy_time, 
            double fuse_time);
    
    std::unique_ptr<SmokeCloud> generate_smoke_cloud() const;
    
    double get_deploy_time() const { return deploy_time_; }
    double get_fuse_time() const { return fuse_time_; }
    double get_detonate_time() const { return detonate_time_; }
    const Vector3d& get_detonate_pos() const { return detonate_pos_; }

private:
    double deploy_time_;
    double fuse_time_;
    double detonate_time_;
    Vector3d detonate_pos_;
};

/**
 * @brief 无人机类
 */
class UAV {
public:
    explicit UAV(const std::string& uav_id);
    
    void set_flight_strategy(double speed, double angle);
    Vector3d get_position(double t) const;
    std::unique_ptr<Grenade> deploy_grenade(double deploy_time, double fuse_time) const;
    
    const std::string& get_id() const { return id_; }
    const Vector3d& get_start_pos() const { return start_pos_; }
    bool is_strategy_set() const { return strategy_set_; }

private:
    std::string id_;
    Vector3d start_pos_;
    double speed_;
    double angle_;
    Vector3d velocity_vec_;
    bool strategy_set_;
};

} // namespace CoreObjects
