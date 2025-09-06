#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <map>
#include <Eigen/Dense>

// --- 物理常量 ---
constexpr double G = 9.8;
constexpr double CLOUD_SINK_SPEED = 3.0;
constexpr double CLOUD_RADIUS = 10.0;
constexpr double CLOUD_DURATION = 20.0;
constexpr double UAV_SPEED_MIN = 70.0;
constexpr double UAV_SPEED_MAX = 140.0;
constexpr double GRENADE_INTERVAL = 1.0;
constexpr double GRENADE_MASS = 5.0;      // 烟幕弹质量 (kg)
constexpr double GRENADE_DRAG_FACTOR = 0.005; // 阻力因子 k = 0.5 * C_d * ρ * A

// --- 目标信息 ---
struct TargetSpecs {
    Eigen::Vector3d center_bottom;
    double radius;
    double height;
};

const TargetSpecs TRUE_TARGET_SPECS = {
    {0.0, 200.0, 0.0}, // center_bottom
    7.0,               // radius
    10.0               // height
};

// --- 初始状态 (t=0) ---
struct MissileSpec {
    Eigen::Vector3d pos;
    double speed;
    Eigen::Vector3d target;
};

struct UavSpec {
    Eigen::Vector3d pos;
};

// 使用静态函数来初始化，以避免静态初始化顺序问题
inline const std::map<std::string, MissileSpec>& get_missiles_initial() {
    static const std::map<std::string, MissileSpec> MISSILES_INITIAL = {
        {"M1", {{20000.0, 0.0, 2000.0}, 300.0, {0.0, 0.0, 0.0}}},
        {"M2", {{19000.0, 600.0, 2100.0}, 300.0, {0.0, 0.0, 0.0}}},
        {"M3", {{18000.0, -600.0, 1900.0}, 300.0, {0.0, 0.0, 0.0}}}
    };
    return MISSILES_INITIAL;
}

inline const std::map<std::string, UavSpec>& get_uavs_initial() {
    static const std::map<std::string, UavSpec> UAVS_INITIAL = {
        {"FY1", {{17800.0, 0.0, 1800.0}}},
        {"FY2", {{12000.0, 1400.0, 1400.0}}},
        {"FY3", {{6000.0, -3000.0, 700.0}}},
        {"FY4", {{11000.0, 2000.0, 1800.0}}},
        {"FY5", {{13000.0, -2000.0, 1300.0}}}
    };
    return UAVS_INITIAL;
}

#endif // CONFIG_HPP
