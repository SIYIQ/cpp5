#include "geometry.hpp"
#include "config.hpp"
#include <cmath>
#include <algorithm> // for std::clamp

// Helper struct to hold cone properties
struct ShadowCone {
    Eigen::Vector3d axis;
    double half_angle;
};

bool check_collective_obscuration(
    const Eigen::Vector3d& missile_pos,
    const std::vector<Eigen::Vector3d>& active_cloud_centers,
    const std::vector<Eigen::Vector3d>& target_key_points
) {
    if (active_cloud_centers.empty()) {
        return false;
    }

    // 1. For each cloud, build its corresponding shadow cone
    std::vector<ShadowCone> cones;
    for (const auto& cloud_center : active_cloud_centers) {
        Eigen::Vector3d vec_vc = cloud_center - missile_pos;
        double dist = vec_vc.norm();

        // If the missile is inside any cloud, it's considered fully obscured
        if (dist <= CLOUD_RADIUS) {
            return true;
        }

        Eigen::Vector3d axis = vec_vc.normalized();
        double angle = std::asin(CLOUD_RADIUS / dist);
        cones.push_back({axis, angle});
    }

    // 2. Check every key point of the target
    // Core logic: For each key point, is it covered by AT LEAST ONE shadow cone?
    for (const auto& point : target_key_points) {
        bool is_point_covered = false;
        for (const auto& cone : cones) {
            Eigen::Vector3d vec_vp = point - missile_pos;
            double vec_vp_norm = vec_vp.norm();

            if (vec_vp_norm < 1e-9) {
                is_point_covered = true;
                break; // Point is at the cone vertex, so it's covered
            }

            double cos_beta = vec_vp.dot(cone.axis) / vec_vp_norm;
            // Clamp the value to [-1.0, 1.0] to avoid domain errors in acos
            double beta = std::acos(std::clamp(cos_beta, -1.0, 1.0));

            if (beta <= cone.half_angle) {
                is_point_covered = true;
                break; // This point is covered, move to the next key point
            }
        }

        // 3. If any key point is not covered by any cone, the obscuration fails
        if (!is_point_covered) {
            return false;
        }
    }

    // 4. If all key points are covered, the collective obscuration is successful
    return true;
}
