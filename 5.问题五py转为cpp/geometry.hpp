#ifndef GEOMETRY_HPP
#define GEOMETRY_HPP

#include <vector>
#include <Eigen/Dense>

bool check_collective_obscuration(
    const Eigen::Vector3d& missile_pos,
    const std::vector<Eigen::Vector3d>& active_cloud_centers,
    const std::vector<Eigen::Vector3d>& target_key_points
);

#endif // GEOMETRY_HPP
