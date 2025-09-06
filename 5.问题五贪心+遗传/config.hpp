#pragma once

#include <array>
#include <unordered_map>
#include <string>
#include <Eigen/Dense>

// Using Eigen for high-performance vector operations
using Vector3d = Eigen::Vector3d;
using Matrix3d = Eigen::Matrix3d;

namespace Config {

// --- 物理常量 ---
constexpr double G = 9.8;
constexpr double CLOUD_SINK_SPEED = 3.0;
constexpr double CLOUD_RADIUS = 10.0;
constexpr double CLOUD_DURATION = 20.0;
constexpr double UAV_SPEED_MIN = 70.0;
constexpr double UAV_SPEED_MAX = 140.0;
constexpr double GRENADE_INTERVAL = 1.0;
constexpr double GRENADE_MASS = 5.0;  // 烟幕弹质量 (kg)
constexpr double GRENADE_DRAG_FACTOR = 0.005; // 阻力因子 k = 0.5 * C_d * ρ * A

// --- 目标信息 ---
struct TargetSpecs {
    Vector3d center_bottom;
    double radius;
    double height;
    
    TargetSpecs() : center_bottom(0.0, 200.0, 0.0), radius(7.0), height(10.0) {}
};

extern const TargetSpecs TRUE_TARGET_SPECS;

// --- 导弹初始状态 ---
struct MissileInitial {
    Vector3d pos;
    double speed;
    Vector3d target;
    
    MissileInitial(const Vector3d& p, double s, const Vector3d& t) 
        : pos(p), speed(s), target(t) {}
};

// --- 无人机初始状态 ---
struct UAVInitial {
    Vector3d pos;
    
    explicit UAVInitial(const Vector3d& p) : pos(p) {}
};

// --- 全局初始化数据 ---
extern const std::unordered_map<std::string, MissileInitial> MISSILES_INITIAL;
extern const std::unordered_map<std::string, UAVInitial> UAVS_INITIAL;

} // namespace Config
