#pragma once

#include <vector>
#include <Eigen/Dense>
#include "config.hpp"

using Vector3d = Eigen::Vector3d;
using Matrix3Xd = Eigen::Matrix3Xd;

namespace Geometry {

/**
 * @brief 阴影锥结构体
 */
struct ShadowCone {
    Vector3d axis;        // 锥轴单位向量
    double half_angle;    // 锥半角 (弧度)
    
    ShadowCone(const Vector3d& a, double angle) : axis(a), half_angle(angle) {}
};

/**
 * @brief 判断在给定时刻，多个烟幕云团是否协同地完全遮蔽了目标
 * 使用覆盖整个目标外表面的关键点集进行判断
 * 
 * @param missile_pos 导弹位置
 * @param active_cloud_centers 所有有效烟幕云团中心的位置列表  
 * @param target_key_points 代表目标外表面的关键点矩阵 (3xN)
 * @return bool 如果协同遮蔽成功，返回 true
 */
bool check_collective_obscuration(
    const Vector3d& missile_pos,
    const std::vector<Vector3d>& active_cloud_centers,
    const Matrix3Xd& target_key_points
);

/**
 * @brief 检查单个点是否在阴影锥内部
 * 
 * @param point 目标点
 * @param missile_pos 导弹位置 (锥顶点)
 * @param cone 阴影锥
 * @return bool 点是否在锥内
 */
bool is_point_in_cone(
    const Vector3d& point,
    const Vector3d& missile_pos,
    const ShadowCone& cone
);

/**
 * @brief 构建阴影锥
 * 
 * @param missile_pos 导弹位置
 * @param cloud_center 烟雾云中心
 * @param cloud_radius 烟雾云半径
 * @return ShadowCone 构建的阴影锥，如果距离过近返回无效锥
 */
std::pair<ShadowCone, bool> build_shadow_cone(
    const Vector3d& missile_pos,
    const Vector3d& cloud_center,
    double cloud_radius = Config::CLOUD_RADIUS
);

} // namespace Geometry
