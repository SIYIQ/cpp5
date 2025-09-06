#include "config.hpp"

// 目标规格定义
const TargetSpecs TRUE_TARGET_SPECS = {
    {0.0, 200.0, 0.0},  // center_bottom
    7.0,                // radius
    10.0                // height
};

// 导弹初始状态定义
const std::unordered_map<std::string, MissileInitialState> MISSILES_INITIAL = {
    {"M1", {{20000.0, 0.0, 2000.0}, 300.0, {0.0, 0.0, 0.0}}},
    {"M2", {{19000.0, 600.0, 2100.0}, 300.0, {0.0, 0.0, 0.0}}},
    {"M3", {{18000.0, -600.0, 1900.0}, 300.0, {0.0, 0.0, 0.0}}}
};

// 无人机初始状态定义
const std::unordered_map<std::string, UAVInitialState> UAVS_INITIAL = {
    {"FY1", {{17800.0, 0.0, 1800.0}}},
    {"FY2", {{12000.0, 1400.0, 1400.0}}},
    {"FY3", {{6000.0, -3000.0, 700.0}}},
    {"FY4", {{11000.0, 2000.0, 1800.0}}},
    {"FY5", {{13000.0, -2000.0, 1300.0}}}
};
