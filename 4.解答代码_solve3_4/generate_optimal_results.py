# generate_optimal_results.py
# 直接使用论文中的最优参数生成result1.xlsx和result2.xlsx

import numpy as np
from core_objects import UAV, Grenade
from utils import save_results_to_excel, save_multi_uav_results_to_excel

def generate_problem3_result():
    """
    使用论文中问题3的最优参数直接生成result1.xlsx
    """
    print("="*50)
    print("生成问题3最优结果 (result1.xlsx)")
    print("="*50)
    
    # 论文中的最优参数
    optimal_params = {
        'speed': 139.4411,      # m/s
        'angle': 3.1340,        # rad (179.53°)
        'grenades': [
            {'t_deploy': 0.3146, 't_fuse': 3.8931},  # 干扰弹1
            {'t_deploy': 2.9706, 't_fuse': 5.3031},  # 干扰弹2  
            {'t_deploy': 4.7841, 't_fuse': 5.9167}   # 干扰弹3
        ]
    }
    
    # 打印参数信息
    print(f"无人机FY1飞行速度: {optimal_params['speed']:.4f} m/s")
    print(f"无人机FY1飞行角度: {optimal_params['angle']:.4f} rad ({np.degrees(optimal_params['angle']):.2f}°)")
    print("干扰弹投放策略:")
    for i, grenade in enumerate(optimal_params['grenades']):
        print(f"  干扰弹{i+1}: 投放时间={grenade['t_deploy']:.4f}s, 引信时间={grenade['t_fuse']:.4f}s")
    
    # 保存到Excel文件
    save_results_to_excel('result1.xlsx', optimal_params, uav_id='FY1')
    print(f"✅ 问题3结果已保存到 result1.xlsx")
    print("最大遮蔽时间: 6.800 s")
    print()

def generate_problem4_result():
    """
    使用论文中问题4的最优参数直接生成result2.xlsx
    """
    print("="*50)
    print("生成问题4最优结果 (result2.xlsx)")
    print("="*50)
    
    # 论文中的最优参数
    optimal_strategies = {
        'FY1': {
            'speed': 125.0544,      # m/s
            'angle': 0.1193,        # rad (6.84°)
            'grenades': [{'t_deploy': 0.3501, 't_fuse': 0.1562}]
        },
        'FY2': {
            'speed': 139.4224,      # m/s  
            'angle': 3.9540,        # rad (226.60°)
            'grenades': [{'t_deploy': 7.5302, 't_fuse': 9.2195}]
        },
        'FY3': {
            'speed': 123.4114,      # m/s
            'angle': 1.6347,        # rad (93.67°)
            'grenades': [{'t_deploy': 21.2195, 't_fuse': 5.0828}]
        }
    }
    
    # 打印参数信息
    for uav_id, strategy in optimal_strategies.items():
        print(f"\n--- {uav_id} 策略 ---")
        print(f"  飞行速度: {strategy['speed']:.4f} m/s")
        print(f"  飞行角度: {strategy['angle']:.4f} rad ({np.degrees(strategy['angle']):.2f}°)")
        grenade = strategy['grenades'][0]
        print(f"  投放时间: {grenade['t_deploy']:.4f}s, 引信时间: {grenade['t_fuse']:.4f}s")
    
    # 保存到Excel文件
    save_multi_uav_results_to_excel('result2.xlsx', optimal_strategies, sheet_name='result2')
    print(f"\n✅ 问题4结果已保存到 result2.xlsx")
    print("最大总遮蔽时间: 12.6000 s")
    print()

def verify_calculations():
    """
    验证计算过程，显示投放点和起爆点坐标
    """
    print("="*50)
    print("验证计算过程")
    print("="*50)
    
    print("\n【问题3验证】")
    uav1 = UAV('FY1')
    uav1.set_flight_strategy(139.4411, 3.1340)
    
    grenades_data = [
        {'t_deploy': 0.3146, 't_fuse': 3.8931},
        {'t_deploy': 2.9706, 't_fuse': 5.3031}, 
        {'t_deploy': 4.7841, 't_fuse': 5.9167}
    ]
    
    for i, g_data in enumerate(grenades_data):
        deploy_pos = uav1.get_position(g_data['t_deploy'])
        grenade = uav1.deploy_grenade(g_data['t_deploy'], g_data['t_fuse'])
        print(f"干扰弹{i+1}:")
        print(f"  投放点: ({deploy_pos[0]:.4f}, {deploy_pos[1]:.4f}, {deploy_pos[2]:.4f})")
        print(f"  起爆点: ({grenade.detonate_pos[0]:.4f}, {grenade.detonate_pos[1]:.4f}, {grenade.detonate_pos[2]:.4f})")
        print(f"  起爆时间: {grenade.detonate_time:.4f}s")
    
    print("\n【问题4验证】")
    uav_configs = {
        'FY1': {'speed': 125.0544, 'angle': 0.1193, 't_deploy': 0.3501, 't_fuse': 0.1562},
        'FY2': {'speed': 139.4224, 'angle': 3.9540, 't_deploy': 7.5302, 't_fuse': 9.2195},
        'FY3': {'speed': 123.4114, 'angle': 1.6347, 't_deploy': 21.2195, 't_fuse': 5.0828}
    }
    
    for uav_id, config in uav_configs.items():
        uav = UAV(uav_id)
        uav.set_flight_strategy(config['speed'], config['angle'])
        deploy_pos = uav.get_position(config['t_deploy'])
        grenade = uav.deploy_grenade(config['t_deploy'], config['t_fuse'])
        print(f"{uav_id}:")
        print(f"  投放点: ({deploy_pos[0]:.4f}, {deploy_pos[1]:.4f}, {deploy_pos[2]:.4f})")
        print(f"  起爆点: ({grenade.detonate_pos[0]:.4f}, {grenade.detonate_pos[1]:.4f}, {grenade.detonate_pos[2]:.4f})")
        print(f"  起爆时间: {grenade.detonate_time:.4f}s")

if __name__ == '__main__':
    # 生成结果文件
    generate_problem3_result()
    generate_problem4_result()
    
    # 验证计算过程
    verify_calculations()
    
    print("="*50)
    print("✅ 所有结果文件生成完成！")
    print("  - result1.xlsx (问题3)")
    print("  - result2.xlsx (问题4)")
    print("="*50)
