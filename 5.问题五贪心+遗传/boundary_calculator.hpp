#pragma once

#include <vector>
#include <string>
#include <Eigen/Dense>
#include "config.hpp"

using Vector3d = Eigen::Vector3d;

namespace BoundaryCalculator {

/**
 * @brief 判断在起爆瞬间，烟云是否在空间上位于导弹和真目标之间
 * 
 * @param cloud_pos 烟云位置
 * @param missile_pos 导弹位置
 * @param target_pos 目标位置
 * @param missile_unit_vec 导弹单位方向向量
 * @return bool 如果烟云在导弹和目标之间，返回 true
 */
bool is_cloud_between_missile_and_target_internal(
    const Vector3d& cloud_pos,
    const Vector3d& missile_pos,
    const Vector3d& target_pos,
    const Vector3d& missile_unit_vec
);

/**
 * @brief 通过分析极端投放策略，找到一个合理的 t_deploy 上边界
 * 
 * @param uav_id 无人机ID，如 'FY1'
 * @param missile_id 导弹ID，如 'M1'
 * @param fuse_time_options 用于测试的极端引信时间列表
 * @return double 计算出的 t_deploy 的最大有效时间
 */
double find_max_effective_deploy_time(
    const std::string& uav_id,
    const std::string& missile_id,
    const std::vector<double>& fuse_time_options = {0.1, 20.0}
);

/**
 * @brief 边界计算结果结构
 */
struct BoundaryResult {
    double t_deploy;
    double angle_degrees;
    Vector3d cloud_pos;
    Vector3d missile_pos;
    bool is_valid;
    double fuse_time;
    
    BoundaryResult(double t, double angle, const Vector3d& c_pos, 
                   const Vector3d& m_pos, bool valid, double fuse)
        : t_deploy(t), angle_degrees(angle), cloud_pos(c_pos), 
          missile_pos(m_pos), is_valid(valid), fuse_time(fuse) {}
};

} // namespace BoundaryCalculator
