#ifndef BOUNDARY_CALCULATOR_HPP
#define BOUNDARY_CALCULATOR_HPP

#include <string>
#include <vector>
#include <array>

/**
 * 判断在起爆瞬间，烟云是否在空间上位于导弹和真目标之间
 */
bool is_cloud_between_missile_and_target_internal(
    const std::array<double, 3>& cloud_pos,
    const std::array<double, 3>& missile_pos,
    const std::array<double, 3>& target_pos,
    const std::array<double, 3>& missile_unit_vec
);

/**
 * 通过分析极端投放策略，找到一个合理的 t_deploy 上边界
 */
double find_max_effective_deploy_time(
    const std::string& uav_id,
    const std::string& missile_id,
    const std::vector<double>& fuse_time_options = {0.1, 20.0}
);

#endif // BOUNDARY_CALCULATOR_HPP
