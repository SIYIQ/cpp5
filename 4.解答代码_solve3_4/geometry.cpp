#include "geometry.hpp"
#include "config.hpp"
#include <cmath>
#include <algorithm>
#include <vector>

struct ShadowCone {
    std::array<double, 3> axis;
    double half_angle;
};

bool check_collective_obscuration(
    const std::array<double, 3>& missile_pos,
    const std::vector<std::array<double, 3>>& active_cloud_centers,
    const std::vector<std::array<double, 3>>& target_key_points
) {
    if (active_cloud_centers.empty()) {
        return false;
    }

    // 1. 为每一个云团构建其对应的阴影锥
    std::vector<ShadowCone> cones;
    for (const auto& cloud_center : active_cloud_centers) {
        // 计算从导弹到云团中心的向量
        std::array<double, 3> vec_vc = {
            cloud_center[0] - missile_pos[0],
            cloud_center[1] - missile_pos[1],
            cloud_center[2] - missile_pos[2]
        };
        
        double dist = std::sqrt(vec_vc[0]*vec_vc[0] + vec_vc[1]*vec_vc[1] + vec_vc[2]*vec_vc[2]);
        
        // 如果导弹在任何一个云团内，视为完全遮蔽
        if (dist <= CLOUD_RADIUS) {
            return true;
        }
        
        // 计算轴向单位向量和半角
        std::array<double, 3> axis = {
            vec_vc[0] / dist,
            vec_vc[1] / dist,
            vec_vc[2] / dist
        };
        double angle = std::asin(CLOUD_RADIUS / dist);
        
        cones.push_back({axis, angle});
    }

    // 2. 检查目标的每一个关键点
    for (const auto& point : target_key_points) {
        bool is_point_covered = false;
        
        for (const auto& cone : cones) {
            // 判断 point 是否在当前 cone 的内部
            std::array<double, 3> vec_vp = {
                point[0] - missile_pos[0],
                point[1] - missile_pos[1],
                point[2] - missile_pos[2]
            };
            
            double vec_vp_norm = std::sqrt(vec_vp[0]*vec_vp[0] + vec_vp[1]*vec_vp[1] + vec_vp[2]*vec_vp[2]);

            if (vec_vp_norm < 1e-9) {
                is_point_covered = true;
                break;
            }

            // 计算点到锥轴的角度
            double cos_beta = (vec_vp[0]*cone.axis[0] + vec_vp[1]*cone.axis[1] + vec_vp[2]*cone.axis[2]) / vec_vp_norm;
            cos_beta = std::max(-1.0, std::min(1.0, cos_beta)); // clamp to [-1, 1]
            double beta = std::acos(cos_beta);
            
            if (beta <= cone.half_angle) {
                is_point_covered = true;
                break; // 这个点被覆盖了，检查下一个点
            }
        }
        
        // 如果发现任何一个关键点没有被任何一个云团覆盖，则协同遮蔽失败
        if (!is_point_covered) {
            return false;
        }
    }
    
    // 如果所有关键点都通过了检查，则协同遮蔽成功
    return true;
}
