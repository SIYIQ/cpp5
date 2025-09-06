#include "geometry.hpp"
#include <cmath>
#include <algorithm>
#include <vector>

namespace Geometry {

bool check_collective_obscuration(
    const Vector3d& missile_pos,
    const std::vector<Vector3d>& active_cloud_centers,
    const Matrix3Xd& target_key_points
) {
    if (active_cloud_centers.empty()) {
        return false;
    }

    // 1. 为每一个云团构建其对应的阴影锥
    std::vector<ShadowCone> cones;
    cones.reserve(active_cloud_centers.size());
    
    for (const auto& cloud_center : active_cloud_centers) {
        Vector3d vec_vc = cloud_center - missile_pos;
        double dist = vec_vc.norm();
        
        // 如果导弹在任何一个云团内，视为完全遮蔽
        if (dist <= Config::CLOUD_RADIUS) {
            return true;
        }
        
        Vector3d axis = vec_vc / dist;
        double angle = std::asin(Config::CLOUD_RADIUS / dist);
        cones.emplace_back(axis, angle);
    }

    // 2. 检查目标的每一个关键点
    // 核心逻辑：对于每一个关键点，检查它是否被"至少一个"阴影锥所包含
    const int num_points = target_key_points.cols();
    
    for (int i = 0; i < num_points; ++i) {
        Vector3d point = target_key_points.col(i);
        
        bool is_point_covered = false;
        for (const auto& cone : cones) {
            if (is_point_in_cone(point, missile_pos, cone)) {
                is_point_covered = true;
                break; // 这个点被覆盖了，立即跳出内层循环，检查下一个点
            }
        }
        
        // 3. 如果发现任何一个关键点没有被任何一个云团覆盖，则协同遮蔽失败
        if (!is_point_covered) {
            return false;
        }
    }
            
    // 4. 如果所有关键点都通过了检查，则协同遮蔽成功
    return true;
}

bool is_point_in_cone(
    const Vector3d& point,
    const Vector3d& missile_pos,
    const ShadowCone& cone
) {
    Vector3d vec_vp = point - missile_pos;
    double vec_vp_norm = vec_vp.norm();

    if (vec_vp_norm < 1e-9) {
        return true; // 点与锥顶重合
    }

    double cos_beta = vec_vp.dot(cone.axis) / vec_vp_norm;
    // 限制在[-1, 1]范围内避免数值误差
    cos_beta = std::clamp(cos_beta, -1.0, 1.0);
    double beta = std::acos(cos_beta);
    
    return beta <= cone.half_angle;
}

std::pair<ShadowCone, bool> build_shadow_cone(
    const Vector3d& missile_pos,
    const Vector3d& cloud_center,
    double cloud_radius
) {
    Vector3d vec_vc = cloud_center - missile_pos;
    double dist = vec_vc.norm();
    
    // 检查距离是否足够大以形成有效的锥
    if (dist <= cloud_radius) {
        // 距离过近，无法形成有效阴影锥
        return {ShadowCone(Vector3d::Zero(), 0.0), false};
    }
    
    Vector3d axis = vec_vc / dist;
    double half_angle = std::asin(cloud_radius / dist);
    
    return {ShadowCone(axis, half_angle), true};
}

} // namespace Geometry
