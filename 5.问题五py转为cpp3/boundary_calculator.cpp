#include "boundary_calculator.hpp"
#include "core_objects.hpp"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <algorithm>

namespace BoundaryCalculator {

bool is_cloud_between_missile_and_target_internal(
    const Vector3d& cloud_pos,
    const Vector3d& missile_pos,
    const Vector3d& target_pos,
    const Vector3d& missile_unit_vec)
{
    // 使用投影判断位置关系
    // 将所有点投影到导弹的飞行轴线上
    // u 是导弹飞行方向单位向量
    const Vector3d& u = missile_unit_vec;
    
    double proj_cloud = cloud_pos.dot(u);
    double proj_missile = missile_pos.dot(u);
    double proj_target = target_pos.dot(u);

    // u 是一个指向原点（假目标）的向量，所以其x,z分量为负
    // 因此，x坐标越大的点，其投影值越小（越负）
    // x_missile > x_cloud => proj_missile < proj_cloud
    // x_cloud > x_target => proj_cloud < proj_target (近似)
    // 正确的顺序应该是 proj_missile < proj_cloud < proj_target
    return proj_missile < proj_cloud && proj_cloud < proj_target;
}

double find_max_effective_deploy_time(
    const std::string& uav_id,
    const std::string& missile_id,
    const std::vector<double>& fuse_time_options)
{
    CoreObjects::UAV uav(uav_id);
    CoreObjects::Missile missile(missile_id);
    CoreObjects::TargetCylinder target(Config::TRUE_TARGET_SPECS);

    std::cout << "开始为 UAV(" << uav_id << ") vs Missile(" << missile_id 
              << ") 计算 t_deploy 的有效上边界..." << std::endl;
    std::cout << std::string(65, '=') << std::endl;
    std::cout << "t_deploy(s) | 飞行角度(°) | 起爆点 X   | 导弹 X     | 是否满足约束?" << std::endl;
    std::cout << std::string(65, '-') << std::endl;

    // 扫描 t_deploy 的值
    for (double t_deploy = 0.1; t_deploy < 40.0; t_deploy += 0.5) {
        
        // 对于给定的 t_deploy，我们需要找到能让烟云最靠前的策略
        // 即无人机飞行速度最大，飞行方向使投放点的x坐标最小
        // 对于FY1和M1，它们y坐标初始几乎同线，最优方向就是沿着x轴负方向飞 (angle=pi)
        double optimal_angle = M_PI;
        uav.set_flight_strategy(Config::UAV_SPEED_MAX, optimal_angle);

        bool is_any_fuse_time_valid = false;
        
        // 测试一组典型的或极端的引信时间
        for (double t_fuse : fuse_time_options) {
            auto grenade = uav.deploy_grenade(t_deploy, t_fuse);
            double t_b = grenade->get_detonate_time();
            
            // 获取关键位置
            Vector3d cloud_pos = grenade->get_detonate_pos();
            Vector3d missile_pos = missile.get_position(t_b);
            
            // 获取导弹的单位方向向量
            Vector3d direction = missile.get_position(1.0) - missile.get_position(0.0);
            Vector3d unit_vec = direction.normalized();
            
            // 检查约束
            if (is_cloud_between_missile_and_target_internal(
                cloud_pos,
                missile_pos,
                target.get_bottom_center(),
                unit_vec
            )) {
                is_any_fuse_time_valid = true;
                
                // 打印一次有效信息即可
                std::cout << std::fixed << std::setprecision(1)
                          << std::setw(11) << t_deploy << " | "
                          << std::setw(12) << (optimal_angle * 180.0 / M_PI) << " | "
                          << std::setw(10) << cloud_pos[0] << " | "
                          << std::setw(10) << missile_pos[0] << " | "
                          << "✔️ (t_fuse=" << t_fuse << "s 有效)" << std::endl;
                break; // 只要有一个t_fuse有效，就认为这个t_deploy是有效的
            }
        }

        if (!is_any_fuse_time_valid) {
            // 如果所有测试的引信时间都无法满足约束，
            // 那么可以认为这个t_deploy已经太大了。
            // 我们取上一个有效的时间点作为边界。
            double max_t_deploy = t_deploy - 0.5; // 回退一个步长
            std::cout << std::fixed << std::setprecision(1)
                      << std::setw(11) << t_deploy << " | "
                      << std::setw(12) << (optimal_angle * 180.0 / M_PI) << " | "
                      << "---        | ---        | ❌  <-- 临界点" << std::endl;
            std::cout << std::string(65, '=') << std::endl;
            std::cout << "结论: 在 t_deploy ≈ " << max_t_deploy 
                      << " 秒之后，即使采用最极端策略也难以形成有效遮蔽。" << std::endl;
            return max_t_deploy;
        }
    }
    
    // 如果扫描完都没找到无效点，说明上界可以设置得更大
    double max_t_deploy_fallback = 40.0;
    std::cout << std::string(65, '=') << std::endl;
    std::cout << "警告: 在扫描范围内未找到无效的 t_deploy，返回默认上界 " 
              << max_t_deploy_fallback << "。" << std::endl;
    return max_t_deploy_fallback;
}

} // namespace BoundaryCalculator
