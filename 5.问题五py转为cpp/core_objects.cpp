#include "core_objects.hpp"
#include <cmath>
#include <stdexcept>
#include <boost/numeric/odeint.hpp>

// --- ODE System for Grenade Trajectory ---
// Define the state type for the ODE: a 6D vector [x, y, z, vx, vy, vz]
using state_type = std::array<double, 6>;

struct GrenadeOde {
    double mass;
    double drag_factor;

    GrenadeOde(double m, double d) : mass(m), drag_factor(d) {}

    void operator()(const state_type& y, state_type& dydt, double /* t */) const {
        Eigen::Vector3d velocity(y[3], y[4], y[5]);
        
        // Gravity
        Eigen::Vector3d gravity_accel(0.0, 0.0, -G);
        
        // Air drag
        Eigen::Vector3d drag_accel = Eigen::Vector3d::Zero();
        double speed = velocity.norm();
        if (speed > 1e-6) {
            drag_accel = -(drag_factor / mass) * speed * velocity;
        }
        
        Eigen::Vector3d total_accel = gravity_accel + drag_accel;

        // Derivatives
        dydt[0] = y[3]; // dx/dt = vx
        dydt[1] = y[4]; // dy/dt = vy
        dydt[2] = y[5]; // dz/dt = vz
        dydt[3] = total_accel.x();
        dydt[4] = total_accel.y();
        dydt[5] = total_accel.z();
    }
};


// --- TargetCylinder Implementation ---
TargetCylinder::TargetCylinder(const TargetSpecs& specs, int num_circ_samples, int num_height_samples)
    : bottom_center_(specs.center_bottom),
      radius_(specs.radius),
      height_(specs.height) {
    top_center_ = bottom_center_ + Eigen::Vector3d(0.0, 0.0, height_);
    _generate_full_key_points(num_circ_samples, num_height_samples);
}

const std::vector<Eigen::Vector3d>& TargetCylinder::get_key_points() const { return key_points_; }
const Eigen::Vector3d& TargetCylinder::get_bottom_center() const { return bottom_center_; }
const Eigen::Vector3d& TargetCylinder::get_top_center() const { return top_center_; }
double TargetCylinder::get_radius() const { return radius_; }
double TargetCylinder::get_height() const { return height_; }

void TargetCylinder::_generate_full_key_points(int num_circ_samples, int num_height_samples) {
    key_points_.clear();
    key_points_.push_back(bottom_center_);
    key_points_.push_back(top_center_);

    for (int i = 0; i < num_circ_samples; ++i) {
        double angle = 2 * M_PI * i / num_circ_samples;
        Eigen::Vector3d offset_xy(radius_ * std::cos(angle), radius_ * std::sin(angle), 0.0);
        key_points_.push_back(bottom_center_ + offset_xy);
        key_points_.push_back(top_center_ + offset_xy);
    }

    int num_side_samples = 4;
    for (int i = 0; i < num_side_samples; ++i) {
        double angle = 2 * M_PI * i / num_side_samples;
        Eigen::Vector3d offset_xy(radius_ * std::cos(angle), radius_ * std::sin(angle), 0.0);
        for (int j = 1; j < num_height_samples; ++j) {
            double height_fraction = static_cast<double>(j) / num_height_samples;
            Eigen::Vector3d height_offset(0.0, 0.0, height_ * height_fraction);
            key_points_.push_back(bottom_center_ + offset_xy + height_offset);
        }
    }
}

// --- Missile Implementation ---
Missile::Missile(const std::string& missile_id) : id_(missile_id) {
    const auto& specs = get_missiles_initial().at(missile_id);
    start_pos_ = specs.pos;
    speed_ = specs.speed;
    Eigen::Vector3d direction_vec = specs.target - start_pos_;
    unit_vec_ = direction_vec.normalized();
}

Eigen::Vector3d Missile::get_position(double t) const {
    return start_pos_ + unit_vec_ * speed_ * t;
}
const std::string& Missile::get_id() const { return id_; }


// --- SmokeCloud Implementation ---
SmokeCloud::SmokeCloud(const Eigen::Vector3d& detonate_pos, double detonate_time)
    : start_time(detonate_time),
      end_time(detonate_time + CLOUD_DURATION),
      detonate_pos_(detonate_pos) {}

std::optional<Eigen::Vector3d> SmokeCloud::get_center(double t) const {
    if (t >= start_time && t < end_time) {
        double t_since_detonate = t - start_time;
        return detonate_pos_ - Eigen::Vector3d(0.0, 0.0, CLOUD_SINK_SPEED * t_since_detonate);
    }
    return std::nullopt;
}

// --- Grenade Implementation ---
Grenade::Grenade(const Eigen::Vector3d& deploy_pos, const Eigen::Vector3d& deploy_vel, double deploy_time, double fuse_time)
    : deploy_time_(deploy_time),
      fuse_time_(fuse_time),
      detonate_time_(deploy_time + fuse_time) {
    detonate_pos_ = _solve_trajectory_odeint(deploy_pos, deploy_vel);
}

Eigen::Vector3d Grenade::_solve_trajectory_odeint(const Eigen::Vector3d& deploy_pos, const Eigen::Vector3d& deploy_vel) {
    state_type initial_state = {
        deploy_pos.x(), deploy_pos.y(), deploy_pos.z(),
        deploy_vel.x(), deploy_vel.y(), deploy_vel.z()
    };

    GrenadeOde ode_system(GRENADE_MASS, GRENADE_DRAG_FACTOR);
    
    // We only need the final state, so we can integrate once.
    // boost::numeric::odeint::integrate(ode_system, initial_state, 0.0, fuse_time_, 0.1); // 0.1 is a suggested step size
    // A more robust way is to use a stepper
    using namespace boost::numeric::odeint;
    auto stepper = make_controlled<runge_kutta_dopri5<state_type>>(1.0e-6, 1.0e-6);
    integrate_adaptive(stepper, ode_system, initial_state, 0.0, fuse_time_, 0.1);

    return Eigen::Vector3d(initial_state[0], initial_state[1], initial_state[2]);
}

SmokeCloud Grenade::generate_smoke_cloud() const {
    return SmokeCloud(detonate_pos_, detonate_time_);
}
const Eigen::Vector3d& Grenade::get_detonate_pos() const { return detonate_pos_; }
double Grenade::get_detonate_time() const { return detonate_time_; }


// --- UAV Implementation ---
UAV::UAV(const std::string& uav_id) : id_(uav_id), speed_(0.0), angle_(0.0) {
    const auto& specs = get_uavs_initial().at(uav_id);
    start_pos_ = specs.pos;
    velocity_vec_.setZero();
}

void UAV::set_flight_strategy(double speed, double angle) {
    speed_ = speed;
    angle_ = angle;
    velocity_vec_ = speed_ * Eigen::Vector3d(std::cos(angle_), std::sin(angle_), 0.0);
}

Eigen::Vector3d UAV::get_position(double t) const {
    if (velocity_vec_.isZero()) {
        // Or handle as an error, depending on desired behavior
        return start_pos_; 
    }
    return start_pos_ + velocity_vec_ * t;
}

Grenade UAV::deploy_grenade(double deploy_time, double fuse_time) {
    if (velocity_vec_.isZero()) {
        throw std::runtime_error("UAV flight strategy has not been set before deploying grenade.");
    }
    Eigen::Vector3d deploy_pos = get_position(deploy_time);
    return Grenade(deploy_pos, velocity_vec_, deploy_time, fuse_time);
}

const Eigen::Vector3d& UAV::get_velocity_vec() const { return velocity_vec_; }
