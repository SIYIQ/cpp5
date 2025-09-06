#include "strategy_calculator.hpp"
#include "core_objects.hpp"
#include <iostream>
#include <iomanip>
#include <stdexcept>

namespace StrategyCalculator {

MultiGrenadeStrategy calculate_points_for_multi_grenade_strategy(
    const std::string& uav_id,
    double speed,
    double angle,
    const std::vector<std::pair<double, double>>& grenade_strategies) {
    
    MultiGrenadeStrategy result(uav_id, speed, angle);
    
    try {
        // 1. 创建无人机实例并设置其飞行策略
        CoreObjects::UAV uav(uav_id);
        uav.set_flight_strategy(speed, angle);

        // 2. 遍历每一枚干扰弹的策略
        for (const auto& [t_deploy, t_fuse] : grenade_strategies) {
            // 计算投放点
            Vector3d deploy_position = uav.get_position(t_deploy);

            // 计算起爆点
            auto grenade = uav.deploy_grenade(t_deploy, t_fuse);
            Vector3d detonate_position = grenade->get_detonate_pos();
            
            result.grenades.emplace_back(t_deploy, t_fuse, deploy_position, detonate_position);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "计算过程中发生错误: " << e.what() << std::endl;
        // 返回空的结果
        result.grenades.clear();
    }
    
    return result;
}

void print_formatted_output_q3(const MultiGrenadeStrategy& strategy) {
    std::cout << std::string(60, '=') << std::endl;
    std::cout << "      问题三 弹道计算器结果" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    std::cout << "输入策略:" << std::endl;
    std::cout << "  - uav_id: " << strategy.uav_id << std::endl;
    std::cout << "  - speed: " << std::fixed << std::setprecision(4) << strategy.speed << std::endl;
    std::cout << "  - angle: " << strategy.angle << std::endl;
    
    std::cout << "\n计算结果:" << std::endl;
    if (strategy.grenades.empty()) {
        std::cout << "  计算失败或无弹药信息。" << std::endl;
    } else {
        for (size_t i = 0; i < strategy.grenades.size(); ++i) {
            const auto& grenade = strategy.grenades[i];
            std::cout << "\n  --- 干扰弹 " << (i + 1) 
                      << " (来自 " << strategy.uav_id << ") ---" << std::endl;
            std::cout << "    - 投放时间: " << std::setprecision(4) << grenade.t_deploy 
                      << "s, 引信时间: " << grenade.t_fuse << "s" << std::endl;
            std::cout << "    - 投放点坐标 (X, Y, Z): (" 
                      << grenade.deploy_pos[0] << ", " 
                      << grenade.deploy_pos[1] << ", " 
                      << grenade.deploy_pos[2] << ")" << std::endl;
            std::cout << "    - 起爆点坐标 (X, Y, Z): (" 
                      << grenade.detonate_pos[0] << ", " 
                      << grenade.detonate_pos[1] << ", " 
                      << grenade.detonate_pos[2] << ")" << std::endl;
        }
    }
    std::cout << std::string(60, '=') << std::endl;
}

bool validate_strategy(const MultiGrenadeStrategy& strategy) {
    if (strategy.uav_id.empty()) {
        std::cerr << "错误: 无人机ID为空" << std::endl;
        return false;
    }
    
    if (strategy.speed < Config::UAV_SPEED_MIN || strategy.speed > Config::UAV_SPEED_MAX) {
        std::cerr << "错误: 无人机速度超出范围 [" 
                  << Config::UAV_SPEED_MIN << ", " << Config::UAV_SPEED_MAX << "]" << std::endl;
        return false;
    }
    
    if (strategy.grenades.empty()) {
        std::cerr << "错误: 没有烟幕弹策略" << std::endl;
        return false;
    }
    
    // 检查时间序列的有效性
    for (size_t i = 0; i < strategy.grenades.size(); ++i) {
        const auto& grenade = strategy.grenades[i];
        
        if (grenade.t_deploy < 0.0) {
            std::cerr << "错误: 烟幕弹 " << (i + 1) << " 的投放时间为负" << std::endl;
            return false;
        }
        
        if (grenade.t_fuse <= 0.0) {
            std::cerr << "错误: 烟幕弹 " << (i + 1) << " 的引信时间无效" << std::endl;
            return false;
        }
        
        // 检查投放时间是否递增
        if (i > 0 && grenade.t_deploy <= strategy.grenades[i-1].t_deploy) {
            std::cerr << "错误: 烟幕弹投放时间必须递增" << std::endl;
            return false;
        }
    }
    
    return true;
}

} // namespace StrategyCalculator
