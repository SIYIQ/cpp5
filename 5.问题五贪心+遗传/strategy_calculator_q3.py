import numpy as np
from core_objects import UAV, Grenade
from config import *

def calculate_points_for_multi_grenade_strategy(strategy):
    """
    根据问题三的策略（单无人机，多弹药），计算每枚弹的投放点和起爆点。
    
    Args:
        strategy (dict): 包含策略详情的字典。
            e.g., {
                'uav_id': 'FY1',
                'speed': 85.3,
                'angle': 3.05,
                'grenades': [
                    {'t_deploy': 10.5, 't_fuse': 15.2},
                    {'t_deploy': 12.0, 't_fuse': 14.8},
                    {'t_deploy': 13.5, 't_fuse': 14.4}
                ]
            }

    Returns:
        list of tuples: [(deploy_pos_1, detonate_pos_1), (deploy_pos_2, detonate_pos_2), ...]
                        返回一个列表，每个元素是一个元组，包含一枚弹的投放点和起爆点坐标。
    """
    results = []
    try:
        # 1. 获取无人机通用策略
        uav_id = strategy['uav_id']
        speed = strategy['speed']
        angle = strategy['angle']

        # 2. 创建无人机实例并设置其飞行策略
        uav = UAV(uav_id)
        uav.set_flight_strategy(speed, angle)

        # 3. 遍历每一枚干扰弹的策略
        for g_strat in strategy['grenades']:
            t_deploy = g_strat['t_deploy']
            t_fuse = g_strat['t_fuse']

            # 计算投放点
            deploy_position = uav.get_position(t_deploy)

            # 计算起爆点
            grenade = uav.deploy_grenade(t_deploy, t_fuse)
            detonate_position = grenade.detonate_pos
            
            results.append((deploy_position, detonate_position))
            
        return results

    except KeyError as e:
        print(f"错误: 策略字典中缺少关键字段: {e}")
        return []
    except Exception as e:
        print(f"计算过程中发生错误: {e}")
        return []

def print_formatted_output_q3(strategy, points_list):
    """为问题三格式化并打印结果。"""
    print("="*60)
    print("      问题三 弹道计算器结果")
    print("="*60)
    print("输入策略:")
    print(f"  - uav_id: {strategy.get('uav_id')}")
    print(f"  - speed: {strategy.get('speed')}")
    print(f"  - angle: {strategy.get('angle')}")
    
    print("\n计算结果:")
    if not points_list:
        print("  计算失败或无弹药信息。")
    else:
        for i, (deploy_pos, detonate_pos) in enumerate(points_list):
            print(f"\n  --- 干扰弹 {i+1} (来自 {strategy.get('uav_id')}) ---")
            print(f"    - 投放时间: {strategy['grenades'][i]['t_deploy']:.4f}s, 引信时间: {strategy['grenades'][i]['t_fuse']:.4f}s")
            print(f"    - 投放点坐标 (X, Y, Z): ({deploy_pos[0]:.4f}, {deploy_pos[1]:.4f}, {deploy_pos[2]:.4f})")
            print(f"    - 起爆点坐标 (X, Y, Z): ({detonate_pos[0]:.4f}, {detonate_pos[1]:.4f}, {detonate_pos[2]:.4f})")
            
    print("="*60)

if __name__ == '__main__':
    # --- 在这里输入你要计算的问题三策略 ---
    # 这应该是一个包含 'uav_id', 'speed', 'angle', 和一个 'grenades' 列表的字典
    # 'grenades' 列表包含三枚弹各自的 't_deploy' 和 't_fuse'
    
    strategy_q3_example = {
        'uav_id': 'FY1',
        'speed': 139.4411, 
        'angle': 3.1340,
        'grenades': [
            {'t_deploy': 0.3146, 't_fuse': 3.8931},
            {'t_deploy': 2.9706, 't_fuse': 5.3031},
            {'t_deploy': 4.7841, 't_fuse': 5.9167}
        ]
    }

    # --- 运行计算并打印 ---
    calculated_points = calculate_points_for_multi_grenade_strategy(strategy_q3_example)
    print_formatted_output_q3(strategy_q3_example, calculated_points)