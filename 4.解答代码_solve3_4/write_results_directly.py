# write_results_directly.py
# 直接向文件夹4中的result1.xlsx和result2.xlsx写入论文中的最优参数数据

import pandas as pd
import numpy as np
from core_objects import UAV, Grenade

def calculate_positions_and_write_result1():
    """
    计算问题3的位置数据并直接写入result1.xlsx
    """
    print("="*60)
    print("写入问题3数据到 result1.xlsx")
    print("="*60)
    
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
    
    print(f"无人机FY1:")
    print(f"  飞行速度: {optimal_params['speed']:.4f} m/s")
    print(f"  飞行角度: {angle_degrees:.2f}° ({optimal_params['angle']:.4f} rad)")
    
    # 准备数据 - 5行10列的完整结构
    data = []
    
    # 前3行：对应3枚干扰弹
    for i, g_strat in enumerate(optimal_params['grenades']):
        # 计算投放点和起爆点
        deploy_pos = uav.get_position(g_strat['t_deploy'])
        grenade = uav.deploy_grenade(g_strat['t_deploy'], g_strat['t_fuse'])
        detonate_pos = grenade.detonate_pos
        
        print(f"\n干扰弹{i+1}:")
        print(f"  投放时间: {g_strat['t_deploy']:.4f}s")
        print(f"  引信时间: {g_strat['t_fuse']:.4f}s")
        print(f"  投放点: ({deploy_pos[0]:.4f}, {deploy_pos[1]:.4f}, {deploy_pos[2]:.4f})")
        print(f"  起爆点: ({detonate_pos[0]:.4f}, {detonate_pos[1]:.4f}, {detonate_pos[2]:.4f})")
        
        # 第一行填写无人机信息和总遮蔽时间，其他行相应字段留空
        if i == 0:
            direction = angle_degrees
            speed = optimal_params['speed']
            total_time = 6.800  # 论文中的遮蔽时间
        else:
            direction = np.nan
            speed = np.nan
            total_time = np.nan
        
        row_data = {
            "无人机运动方向": direction,
            "无人机运动速度 (m/s)": speed,
            "烟幕干扰弹编号": float(i + 1),
            "烟幕干扰弹投放点的x坐标 (m)": deploy_pos[0],
            "烟幕干扰弹投放点的y坐标 (m)": deploy_pos[1],
            "烟幕干扰弹投放点的z坐标 (m)": deploy_pos[2],
            "烟幕干扰弹起爆点的x坐标 (m)": detonate_pos[0],
            "烟幕干扰弹起爆点的y坐标 (m)": detonate_pos[1],
            "烟幕干扰弹起爆点的z坐标 (m)": detonate_pos[2],
            "有效干扰时长 (s)": total_time
        }
        data.append(row_data)
    
    # 第4行：全空行
    empty_row = {
        "无人机运动方向": np.nan,
        "无人机运动速度 (m/s)": np.nan,
        "烟幕干扰弹编号": np.nan,
        "烟幕干扰弹投放点的x坐标 (m)": np.nan,
        "烟幕干扰弹投放点的y坐标 (m)": np.nan,
        "烟幕干扰弹投放点的z坐标 (m)": np.nan,
        "烟幕干扰弹起爆点的x坐标 (m)": np.nan,
        "烟幕干扰弹起爆点的y坐标 (m)": np.nan,
        "烟幕干扰弹起爆点的z坐标 (m)": np.nan,
        "有效干扰时长 (s)": np.nan
    }
    data.append(empty_row)
    
    # 第5行：注释行
    note_row = {
        "无人机运动方向": "注：以x轴为正向，逆时针方向为正，取值0~360（度）。",
        "无人机运动速度 (m/s)": np.nan,
        "烟幕干扰弹编号": np.nan,
        "烟幕干扰弹投放点的x坐标 (m)": np.nan,
        "烟幕干扰弹投放点的y坐标 (m)": np.nan,
        "烟幕干扰弹投放点的z坐标 (m)": np.nan,
        "烟幕干扰弹起爆点的x坐标 (m)": np.nan,
        "烟幕干扰弹起爆点的y坐标 (m)": np.nan,
        "烟幕干扰弹起爆点的z坐标 (m)": np.nan,
        "有效干扰时长 (s)": np.nan
    }
    data.append(note_row)
    
    # 创建DataFrame并写入文件
    df = pd.DataFrame(data)
    df.to_excel('result1.xlsx', index=False, sheet_name='Sheet1')
    
    print(f"\n✅ 数据已写入 result1.xlsx")
    print(f"论文中的遮蔽时间: 6.800 s")

def calculate_positions_and_write_result2():
    """
    计算问题4的位置数据并直接写入result2.xlsx
    """
    print("\n" + "="*60)
    print("写入问题4数据到 result2.xlsx")
    print("="*60)
    
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
    
    # 准备数据
    data = []
    
    # 前3行：对应3架无人机
    for idx, (uav_id, strategy) in enumerate(optimal_strategies.items()):
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
        
        print(f"\n{uav_id}:")
        print(f"  飞行速度: {strategy['speed']:.4f} m/s")
        print(f"  飞行角度: {angle_degrees:.2f}° ({strategy['angle']:.4f} rad)")
        print(f"  投放时间: {g_strat['t_deploy']:.4f}s")
        print(f"  引信时间: {g_strat['t_fuse']:.4f}s")
        print(f"  投放点: ({deploy_pos[0]:.4f}, {deploy_pos[1]:.4f}, {deploy_pos[2]:.4f})")
        print(f"  起爆点: ({detonate_pos[0]:.4f}, {detonate_pos[1]:.4f}, {detonate_pos[2]:.4f})")
        
        # 第一行填写总遮蔽时间，其他行留空
        total_time = 12.6000 if idx == 0 else np.nan
        
        row_data = {
            "无人机编号": uav_id,
            "无人机运动方向": angle_degrees,
            "无人机运动速度 (m/s)": strategy['speed'],
            "烟幕干扰弹投放点的x坐标 (m)": deploy_pos[0],
            "烟幕干扰弹投放点的y坐标 (m)": deploy_pos[1],
            "烟幕干扰弹投放点的z坐标 (m)": deploy_pos[2],
            "烟幕干扰弹起爆点的x坐标 (m)": detonate_pos[0],
            "烟幕干扰弹起爆点的y坐标 (m)": detonate_pos[1],
            "烟幕干扰弹起爆点的z坐标 (m)": detonate_pos[2],
            "有效干扰时长 (s)": total_time
        }
        data.append(row_data)
    
    # 第4行：全空行
    empty_row = {
        "无人机编号": np.nan,
        "无人机运动方向": np.nan,
        "无人机运动速度 (m/s)": np.nan,
        "烟幕干扰弹投放点的x坐标 (m)": np.nan,
        "烟幕干扰弹投放点的y坐标 (m)": np.nan,
        "烟幕干扰弹投放点的z坐标 (m)": np.nan,
        "烟幕干扰弹起爆点的x坐标 (m)": np.nan,
        "烟幕干扰弹起爆点的y坐标 (m)": np.nan,
        "烟幕干扰弹起爆点的z坐标 (m)": np.nan,
        "有效干扰时长 (s)": np.nan
    }
    data.append(empty_row)
    
    # 第5行：注释行
    note_row = {
        "无人机编号": np.nan,
        "无人机运动方向": "注：以x轴为正向，逆时针方向为正，取值0~360（度）。",
        "无人机运动速度 (m/s)": np.nan,
        "烟幕干扰弹投放点的x坐标 (m)": np.nan,
        "烟幕干扰弹投放点的y坐标 (m)": np.nan,
        "烟幕干扰弹投放点的z坐标 (m)": np.nan,
        "烟幕干扰弹起爆点的x坐标 (m)": np.nan,
        "烟幕干扰弹起爆点的y坐标 (m)": np.nan,
        "烟幕干扰弹起爆点的z坐标 (m)": np.nan,
        "有效干扰时长 (s)": np.nan
    }
    data.append(note_row)
    
    # 创建DataFrame并写入文件
    df = pd.DataFrame(data)
    df.to_excel('result2.xlsx', index=False, sheet_name='Sheet1')
    
    print(f"\n✅ 数据已写入 result2.xlsx")
    print(f"论文中的遮蔽时间: 12.6000 s")

if __name__ == '__main__':
    print("直接向文件夹4的官方Excel文件写入论文最优参数")
    print("="*60)
    
    # 写入数据
    calculate_positions_and_write_result1()
    calculate_positions_and_write_result2()
    
    print("\n" + "="*60)
    print("✅ 所有数据已成功写入官方Excel文件！")
    print("  - result1.xlsx (问题3) - 已更新")
    print("  - result2.xlsx (问题4) - 已更新")
    print("✅ 格式完全匹配现有文件结构")
    print("✅ 数据来源于论文中的最优参数")
    print("="*60)
