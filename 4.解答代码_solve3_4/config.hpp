#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <vector>
#include <array>
#include <unordered_map>
#include <string>

// 物理常量
constexpr double G = 9.8;
constexpr double CLOUD_SINK_SPEED = 3.0;
constexpr double CLOUD_RADIUS = 10.0;
constexpr double CLOUD_DURATION = 20.0;
constexpr double UAV_SPEED_MIN = 70.0;
constexpr double UAV_SPEED_MAX = 140.0;
constexpr double GRENADE_INTERVAL = 1.0;
constexpr double GRENADE_MASS = 5.0;        // 烟幕弹质量 (kg)
constexpr double GRENADE_DRAG_FACTOR = 0.005; // 阻力因子

// 目标规格
struct TargetSpecs {
    std::array<double, 3> center_bottom;
    double radius;
    double height;
};

extern const TargetSpecs TRUE_TARGET_SPECS;

// 导弹初始状态
struct MissileInitialState {
    std::array<double, 3> pos;
    double speed;
    std::array<double, 3> target;
};

extern const std::unordered_map<std::string, MissileInitialState> MISSILES_INITIAL;

// 无人机初始状态
struct UAVInitialState {
    std::array<double, 3> pos;
};

extern const std::unordered_map<std::string, UAVInitialState> UAVS_INITIAL;

#endif // CONFIG_HPP
