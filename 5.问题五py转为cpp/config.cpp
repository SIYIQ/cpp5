#include "config.hpp"

namespace Config {

// --- 目标信息实例 ---
const TargetSpecs TRUE_TARGET_SPECS;

// --- 导弹初始状态 ---
const std::unordered_map<std::string, MissileInitial> MISSILES_INITIAL = {
    {"M1", MissileInitial(Vector3d(20000.0, 0.0, 2000.0), 300.0, Vector3d(0.0, 0.0, 0.0))},
    {"M2", MissileInitial(Vector3d(19000.0, 600.0, 2100.0), 300.0, Vector3d(0.0, 0.0, 0.0))},
    {"M3", MissileInitial(Vector3d(18000.0, -600.0, 1900.0), 300.0, Vector3d(0.0, 0.0, 0.0))}
};

// --- 无人机初始状态 ---
const std::unordered_map<std::string, UAVInitial> UAVS_INITIAL = {
    {"FY1", UAVInitial(Vector3d(17800.0, 0.0, 1800.0))},
    {"FY2", UAVInitial(Vector3d(12000.0, 1400.0, 1400.0))},
    {"FY3", UAVInitial(Vector3d(6000.0, -3000.0, 700.0))},
    {"FY4", UAVInitial(Vector3d(11000.0, 2000.0, 1800.0))},
    {"FY5", UAVInitial(Vector3d(13000.0, -2000.0, 1300.0))}
};

} // namespace Config
