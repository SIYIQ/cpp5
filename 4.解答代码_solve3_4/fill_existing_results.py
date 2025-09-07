# fill_existing_results.py
# 只填充数据到现有的result1.xlsx和result2.xlsx，不改变原有结构

import pandas as pd
import numpy as np
from core_objects import UAV, Grenade

def fill_result1_data():
    """
    只向现有的result1.xlsx填充计算数据，保持原有结构不变
    """
    print("="*60)
    print("填充数据到现有的 result1.xlsx")
    print("="*60)
    
    # 读取现有文件
    df = pd.read_excel('result1.xlsx')
    print("原始文件结构:")
    print(df.to_string())
    
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
    
    # 创建无人机并设置策略
    uav = UAV('FY1')
    uav.set_flight_strategy(optimal_params['speed'], optimal_params['angle'])
    
    # 转换角度为度数
    angle_degrees = np.degrees(optimal_params['angle'])
    if angle_degrees < 0:
        angle_degrees += 360
    angle_degrees = round(angle_degrees, 2)
    
    print(f"\n计算参数:")
    print(f"无人机FY1 - 速度: {optimal_params['speed']:.4f} m/s, 角度: {angle_degrees:.2f}°")
    
    # 只在第0行填写无人机信息
    df.iloc[0, df.columns.get_loc('无人机运动方向')] = angle_degrees
    df.iloc[0, df.columns.get_loc('无人机运动速度 (m/s)')] = optimal_params['speed']
    df.iloc[0, df.columns.get_loc('有效干扰时长 (s)')] = 6.800  # 论文中的总遮蔽时间
    
    # 为每枚干扰弹填充坐标数据
    for i, g_strat in enumerate(optimal_params['grenades']):
        # 计算投放点和起爆点
        deploy_pos = uav.get_position(g_strat['t_deploy'])
        grenade = uav.deploy_grenade(g_strat['t_deploy'], g_strat['t_fuse'])
        detonate_pos = grenade.detonate_pos
        
        print(f"干扰弹{i+1}: 投放点({deploy_pos[0]:.4f}, {deploy_pos[1]:.4f}, {deploy_pos[2]:.4f}), "
              f"起爆点({detonate_pos[0]:.4f}, {detonate_pos[1]:.4f}, {detonate_pos[2]:.4f})")
        
        # 填充第i行的坐标数据
        df.iloc[i, df.columns.get_loc('烟幕干扰弹投放点的x坐标 (m)')] = deploy_pos[0]
        df.iloc[i, df.columns.get_loc('烟幕干扰弹投放点的y坐标 (m)')] = deploy_pos[1]
        df.iloc[i, df.columns.get_loc('烟幕干扰弹投放点的z坐标 (m)')] = deploy_pos[2]
        df.iloc[i, df.columns.get_loc('烟幕干扰弹起爆点的x坐标 (m)')] = detonate_pos[0]
        df.iloc[i, df.columns.get_loc('烟幕干扰弹起爆点的y坐标 (m)')] = detonate_pos[1]
        df.iloc[i, df.columns.get_loc('烟幕干扰弹起爆点的z坐标 (m)')] = detonate_pos[2]
    
    # 保存文件
    df.to_excel('result1.xlsx', index=False)
    
    print(f"\n✅ 数据已填充到 result1.xlsx")
    print("填充后文件内容:")
    print(df.to_string())

def fill_result2_data():
    """
    只向现有的result2.xlsx填充计算数据，保持原有结构不变
    """
    print("\n" + "="*60)
    print("填充数据到现有的 result2.xlsx")
    print("="*60)
    
    # 读取现有文件
    df = pd.read_excel('result2.xlsx')
    print("原始文件结构:")
    print(df.to_string())
    
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
    
    print(f"\n计算参数:")
    
    # 只在第0行填写总遮蔽时间
    df.iloc[0, df.columns.get_loc('有效干扰时长 (s)')] = 12.6000  # 论文中的总遮蔽时间
    
    # 为每架无人机填充数据
    for i, (uav_id, strategy) in enumerate(optimal_strategies.items()):
        # 创建无人机
        uav = UAV(uav_id)
        uav.set_flight_strategy(strategy['speed'], strategy['angle'])
        
        # 转换角度
        angle_degrees = np.degrees(strategy['angle'])
        if angle_degrees < 0:
            angle_degrees += 360
        angle_degrees = round(angle_degrees, 2)
        
        # 计算投放点和起爆点
        g_strat = strategy['grenades'][0]
        deploy_pos = uav.get_position(g_strat['t_deploy'])
        grenade = uav.deploy_grenade(g_strat['t_deploy'], g_strat['t_fuse'])
        detonate_pos = grenade.detonate_pos
        
        print(f"{uav_id}: 速度{strategy['speed']:.4f} m/s, 角度{angle_degrees:.2f}°, "
              f"投放点({deploy_pos[0]:.4f}, {deploy_pos[1]:.4f}, {deploy_pos[2]:.4f}), "
              f"起爆点({detonate_pos[0]:.4f}, {detonate_pos[1]:.4f}, {detonate_pos[2]:.4f})")
        
        # 填充第i行的数据（无人机编号已存在，只填充其他字段）
        df.iloc[i, df.columns.get_loc('无人机运动方向')] = angle_degrees
        df.iloc[i, df.columns.get_loc('无人机运动速度 (m/s)')] = strategy['speed']
        df.iloc[i, df.columns.get_loc('烟幕干扰弹投放点的x坐标 (m)')] = deploy_pos[0]
        df.iloc[i, df.columns.get_loc('烟幕干扰弹投放点的y坐标 (m)')] = deploy_pos[1]
        df.iloc[i, df.columns.get_loc('烟幕干扰弹投放点的z坐标 (m)')] = deploy_pos[2]
        df.iloc[i, df.columns.get_loc('烟幕干扰弹起爆点的x坐标 (m)')] = detonate_pos[0]
        df.iloc[i, df.columns.get_loc('烟幕干扰弹起爆点的y坐标 (m)')] = detonate_pos[1]
        df.iloc[i, df.columns.get_loc('烟幕干扰弹起爆点的z坐标 (m)')] = detonate_pos[2]
    
    # 保存文件
    df.to_excel('result2.xlsx', index=False)
    
    print(f"\n✅ 数据已填充到 result2.xlsx")
    print("填充后文件内容:")
    print(df.to_string())

if __name__ == '__main__':
    print("向现有Excel文件填充论文最优参数数据")
    print("="*60)
    print("⚠️  注意：只填充数据，不改变原有文件结构")
    
    # 填充数据
    fill_result1_data()
    fill_result2_data()
    
    print("\n" + "="*60)
    print("✅ 所有数据填充完成！")
    print("  - result1.xlsx - 保持原结构，只填充计算数据")
    print("  - result2.xlsx - 保持原结构，只填充计算数据")
    print("  - 烟幕干扰弹编号和无人机编号保持不变")
    print("  - 注释行保持不变")
    print("="*60)
