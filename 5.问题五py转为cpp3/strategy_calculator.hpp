#pragma once

#include <vector>
#include <string>
#include <Eigen/Dense>
#include "config.hpp"

using Vector3d = Eigen::Vector3d;

namespace StrategyCalculator {

/**
 * @brief 单个烟幕弹的投放和起爆信息
 */
struct GrenadeInfo {
    double t_deploy;
    double t_fuse;
    Vector3d deploy_pos;
    Vector3d detonate_pos;
    
    GrenadeInfo(double deploy_time, double fuse_time, 
                const Vector3d& deploy_position, const Vector3d& detonate_position)
        : t_deploy(deploy_time), t_fuse(fuse_time), 
          deploy_pos(deploy_position), detonate_pos(detonate_position) {}
};

/**
 * @brief 单无人机多弹药策略结构
 */
struct MultiGrenadeStrategy {
    std::string uav_id;
    double speed;
    double angle;
    std::vector<GrenadeInfo> grenades;
    
    MultiGrenadeStrategy(const std::string& id, double spd, double ang)
        : uav_id(id), speed(spd), angle(ang) {}
};

/**
 * @brief 根据问题三的策略（单无人机，多弹药），计算每枚弹的投放点和起爆点
 * 
 * @param uav_id 无人机ID
 * @param speed 飞行速度
 * @param angle 飞行角度
 * @param grenade_strategies 烟幕弹投放策略列表
 * @return MultiGrenadeStrategy 完整的策略信息，包含计算后的位置
 */
MultiGrenadeStrategy calculate_points_for_multi_grenade_strategy(
    const std::string& uav_id,
    double speed,
    double angle,
    const std::vector<std::pair<double, double>>& grenade_strategies // (t_deploy, t_fuse) pairs
);

/**
 * @brief 格式化并打印问题三的计算结果
 * 
 * @param strategy 计算完成的策略
 */
void print_formatted_output_q3(const MultiGrenadeStrategy& strategy);

/**
 * @brief 验证策略的有效性
 * 
 * @param strategy 待验证的策略
 * @return bool 策略是否有效
 */
bool validate_strategy(const MultiGrenadeStrategy& strategy);

} // namespace StrategyCalculator
