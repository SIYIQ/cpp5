#include "boundary_calculator.hpp"
#include "core_objects.hpp"
#include "config.hpp"
#define _USE_MATH_DEFINES
#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <string>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

bool is_cloud_between_missile_and_target_internal(
    const std::array<double, 3>& cloud_pos,
    const std::array<double, 3>& missile_pos,
    const std::array<double, 3>& target_pos,
    const std::array<double, 3>& missile_unit_vec
) {
    // 使用投影判断位置关系
    // 将所有点投影到导弹的飞行轴线上
    
    double proj_cloud = cloud_pos[0] * missile_unit_vec[0] + 
                       cloud_pos[1] * missile_unit_vec[1] + 
                       cloud_pos[2] * missile_unit_vec[2];
                       
    double proj_missile = missile_pos[0] * missile_unit_vec[0] + 
                         missile_pos[1] * missile_unit_vec[1] + 
                         missile_pos[2] * missile_unit_vec[2];
                         
    double proj_target = target_pos[0] * missile_unit_vec[0] + 
                        target_pos[1] * missile_unit_vec[1] + 
                        target_pos[2] * missile_unit_vec[2];

    // 正确的顺序应该是 proj_missile < proj_cloud < proj_target
    return proj_missile < proj_cloud && proj_cloud < proj_target;
}

double find_max_effective_deploy_time(
    const std::string& uav_id,
    const std::string& missile_id,
    const std::vector<double>& fuse_time_options
) {
    UAV uav(uav_id);
    Missile missile(missile_id);
    TargetCylinder target(TRUE_TARGET_SPECS);

    std::cout << "开始为 UAV(" << uav_id << ") vs Missile(" << missile_id 
              << ") 计算 t_deploy 的有效上边界..." << std::endl;
    std::cout << std::string(65, '=') << std::endl;
    std::cout << "t_deploy(s) | 飞行角度(°) | 起爆点 X   | 导弹 X     | 是否满足约束?" << std::endl;
    std::cout << std::string(65, '-') << std::endl;

    // 扫描 t_deploy 的值
    for (double t_deploy = 0.1; t_deploy < 40.0; t_deploy += 0.5) {
        
        // 对于给定的 t_deploy，使用最极端的策略
        // 无人机飞行速度最大，飞行方向使投放点的x坐标最小
        double optimal_angle = M_PI; // 沿着x轴负方向飞
        uav.set_flight_strategy(UAV_SPEED_MAX, optimal_angle);

        bool is_any_fuse_time_valid = false;
        
        // 测试一组引信时间
        for (double t_fuse : fuse_time_options) {
            auto grenade = uav.deploy_grenade(t_deploy, t_fuse);
            double t_b = grenade->detonate_time;
            
            // 获取关键位置
            auto cloud_pos = grenade->detonate_pos;
            auto missile_pos = missile.get_position(t_b);
            
            // 检查约束
            if (is_cloud_between_missile_and_target_internal(
                cloud_pos, missile_pos, target.bottom_center, missile.unit_vec
            )) {
                is_any_fuse_time_valid = true;
                
                // 打印有效信息
                std::cout << std::setw(11) << std::setprecision(1) << std::fixed << t_deploy << " | "
                          << std::setw(12) << std::setprecision(1) << (optimal_angle * 180.0 / M_PI) << " | "
                          << std::setw(10) << std::setprecision(1) << cloud_pos[0] << " | "
                          << std::setw(10) << std::setprecision(1) << missile_pos[0] << " | "
                          << "✔️ (t_fuse=" << std::setprecision(1) << t_fuse << "s 有效)" << std::endl;
                break; // 只要有一个t_fuse有效，就认为这个t_deploy是有效的
            }
        }

        if (!is_any_fuse_time_valid) {
            // 如果所有测试的引信时间都无法满足约束，
            // 那么可以认为这个t_deploy已经太大了
            double max_t_deploy = t_deploy - 0.5; // 回退一个步长
            
            std::cout << std::setw(11) << std::setprecision(1) << std::fixed << t_deploy << " | "
                      << std::setw(12) << std::setprecision(1) << (optimal_angle * 180.0 / M_PI) << " | "
                      << "---        | ---        | ❌  <-- 临界点" << std::endl;
            std::cout << std::string(65, '=') << std::endl;
            std::cout << "结论: 在 t_deploy ≈ " << std::setprecision(1) << max_t_deploy 
                      << " 秒之后，即使采用最极端策略也难以形成有效遮蔽。" << std::endl;
            
            return max_t_deploy;
        }
    }
    
    // 如果扫描完都没找到无效点，说明上界可以设置得更大
    double max_t_deploy_fallback = 40.0;
    std::cout << std::string(65, '=') << std::endl;
    std::cout << "警告: 在扫描范围内未找到无效的 t_deploy，返回默认上界 " 
              << std::setprecision(1) << max_t_deploy_fallback << "。" << std::endl;
    
    return max_t_deploy_fallback;
}

#ifdef STANDALONE_TOOL
// 独立工具主函数
int main() {
    try {
        // 针对问题3、4中的 FY1 vs M1 场景
        double constrained_t_deploy_max = find_max_effective_deploy_time("FY1", "M1");
        
        std::cout << "\n[最终建议] 将 t_deploy 的上边界设置为: " 
                  << std::fixed << std::setprecision(2) << constrained_t_deploy_max << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
#endif // STANDALONE_TOOL
