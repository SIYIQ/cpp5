#ifndef CORE_OBJECTS_HPP
#define CORE_OBJECTS_HPP

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <Eigen/Dense>
#include "config.hpp"

// Forward declarations
class Missile;
class SmokeCloud;
class Grenade;
class UAV;

class TargetCylinder {
public:
    TargetCylinder(const TargetSpecs& specs, int num_circ_samples = 16, int num_height_samples = 5);

    const std::vector<Eigen::Vector3d>& get_key_points() const;
    const Eigen::Vector3d& get_bottom_center() const;
    const Eigen::Vector3d& get_top_center() const;
    double get_radius() const;
    double get_height() const;

private:
    void _generate_full_key_points(int num_circ_samples, int num_height_samples);

    Eigen::Vector3d bottom_center_;
    Eigen::Vector3d top_center_;
    double radius_;
    double height_;
    std::vector<Eigen::Vector3d> key_points_;
};

class Missile {
public:
    explicit Missile(const std::string& missile_id);

    Eigen::Vector3d get_position(double t) const;
    const std::string& get_id() const;

private:
    std::string id_;
    Eigen::Vector3d start_pos_;
    double speed_;
    Eigen::Vector3d unit_vec_;
};

class SmokeCloud {
public:
    SmokeCloud(const Eigen::Vector3d& detonate_pos, double detonate_time);

    std::optional<Eigen::Vector3d> get_center(double t) const;
    
    double start_time;
    double end_time;
    std::string target_missile_id; // For global optimizer

private:
    Eigen::Vector3d detonate_pos_;
};

class Grenade {
public:
    Grenade(const Eigen::Vector3d& deploy_pos, const Eigen::Vector3d& deploy_vel, double deploy_time, double fuse_time);

    SmokeCloud generate_smoke_cloud() const;
    const Eigen::Vector3d& get_detonate_pos() const;
    double get_detonate_time() const;

private:
    Eigen::Vector3d _solve_trajectory_odeint(const Eigen::Vector3d& deploy_pos, const Eigen::Vector3d& deploy_vel);

    double deploy_time_;
    double fuse_time_;
    double detonate_time_;
    Eigen::Vector3d detonate_pos_;
};

class UAV {
public:
    explicit UAV(const std::string& uav_id);

    void set_flight_strategy(double speed, double angle);
    Eigen::Vector3d get_position(double t) const;
    Grenade deploy_grenade(double deploy_time, double fuse_time);
    const Eigen::Vector3d& get_velocity_vec() const;

private:
    std::string id_;
    Eigen::Vector3d start_pos_;
    double speed_;
    double angle_;
    Eigen::Vector3d velocity_vec_;
};

#endif // CORE_OBJECTS_HPP
