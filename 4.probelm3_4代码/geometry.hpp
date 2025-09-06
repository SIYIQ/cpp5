#ifndef GEOMETRY_HPP
#define GEOMETRY_HPP

#include <vector>
#include <array>

/**
 * 判断在给定时刻，多个烟幕云团是否协同地完全遮蔽了目标。
 * 使用覆盖整个目标外表面的关键点集进行判断。
 */
bool check_collective_obscuration(
    const std::array<double, 3>& missile_pos,
    const std::vector<std::array<double, 3>>& active_cloud_centers,
    const std::vector<std::array<double, 3>>& target_key_points
);

#endif // GEOMETRY_HPP
