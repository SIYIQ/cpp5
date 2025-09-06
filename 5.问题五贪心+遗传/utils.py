##任务三
# utils.py
import pandas as pd
from core_objects import UAV, Grenade

def save_results_to_excel(filename, strategy, uav_id='FY1'):
    """
    将单架无人机的多枚弹药策略保存到指定的Excel文件中。
    """
    uav = UAV(uav_id)
    uav.set_flight_strategy(strategy['speed'], strategy['angle'])
    
    data = []
    
    # 无人机飞行策略
    uav_data = {
        "对象": f"无人机 {uav_id}",
        "飞行速度 (m/s)": f"{strategy['speed']:.4f}",
        "飞行方向 (rad)": f"{strategy['angle']:.4f}",
        "投放点 X": "", "投放点 Y": "", "投放点 Z": "",
        "起爆点 X": "", "起爆点 Y": "", "起爆点 Z": ""
    }
    data.append(uav_data)
    
    # 各枚干扰弹的策略
    for i, g_strat in enumerate(strategy['grenades']):
        t_deploy = g_strat['t_deploy']
        t_fuse = g_strat['t_fuse']
        
        deploy_pos = uav.get_position(t_deploy)
        grenade = uav.deploy_grenade(t_deploy, t_fuse)
        detonate_pos = grenade.detonate_pos

        grenade_data = {
            "对象": f"干扰弹 {i+1}",
            "飞行速度 (m/s)": "",
            "飞行方向 (rad)": "",
            "投放点 X": f"{deploy_pos[0]:.4f}",
            "投放点 Y": f"{deploy_pos[1]:.4f}",
            "投放点 Z": f"{deploy_pos[2]:.4f}",
            "起爆点 X": f"{detonate_pos[0]:.4f}",
            "起爆点 Y": f"{detonate_pos[1]:.4f}",
            "起爆点 Z": f"{detonate_pos[2]:.4f}",
        }
        data.append(grenade_data)
        
    df = pd.DataFrame(data)
    
    # 调整列顺序以匹配模板
    column_order = [
        "对象", "飞行速度 (m/s)", "飞行方向 (rad)", 
        "投放点 X", "投放点 Y", "投放点 Z", 
        "起爆点 X", "起爆点 Y", "起爆点 Z"
    ]
    df = df[column_order]
    
    try:
        df.to_excel(filename, index=False, sheet_name='result1')
        print(f"结果已成功保存到 {filename}")
    except Exception as e:
        print(f"保存Excel失败: {e}")

# ##任务四
# # utils.py
# import pandas as pd
# from core_objects import UAV, Grenade

# def save_multi_uav_results_to_excel(filename, strategies, sheet_name='result'):
#     """
#     将多架无人机的策略保存到指定的Excel文件中。
#     strategies: 一个字典，键是uav_id, 值是该无人机的策略字典。
#                 e.g., {'FY1': {'speed': ..., 'grenades': [...]}, 'FY2': ...}
#     """
#     data = []
    
#     # 按无人机ID排序，保证输出顺序稳定
#     sorted_uav_ids = sorted(strategies.keys())

#     for uav_id in sorted_uav_ids:
#         strategy = strategies[uav_id]
#         uav = UAV(uav_id)
#         uav.set_flight_strategy(strategy['speed'], strategy['angle'])
        
#         # 无人机飞行策略
#         uav_data = {
#             "对象": f"无人机 {uav_id}",
#             "飞行速度 (m/s)": f"{strategy['speed']:.4f}",
#             "飞行方向 (rad)": f"{strategy['angle']:.4f}",
#             "投放点 X": "", "投放点 Y": "", "投放点 Z": "",
#             "起爆点 X": "", "起爆点 Y": "", "起爆点 Z": ""
#         }
#         data.append(uav_data)
        
#         # 该无人机投放的干扰弹策略 (问题四中每架只有1枚)
#         for i, g_strat in enumerate(strategy['grenades']):
#             t_deploy = g_strat['t_deploy']
#             t_fuse = g_strat['t_fuse']
            
#             deploy_pos = uav.get_position(t_deploy)
#             grenade = uav.deploy_grenade(t_deploy, t_fuse)
#             detonate_pos = grenade.detonate_pos

#             grenade_data = {
#                 "对象": f"干扰弹 ({uav_id}-{i+1})", # 标记是哪个无人机投的
#                 "飞行速度 (m/s)": "",
#                 "飞行方向 (rad)": "",
#                 "投放点 X": f"{deploy_pos[0]:.4f}",
#                 "投放点 Y": f"{deploy_pos[1]:.4f}",
#                 "投放点 Z": f"{deploy_pos[2]:.4f}",
#                 "起爆点 X": f"{detonate_pos[0]:.4f}",
#                 "起爆点 Y": f"{detonate_pos[1]:.4f}",
#                 "起爆点 Z": f"{detonate_pos[2]:.4f}",
#             }
#             data.append(grenade_data)
            
#     df = pd.DataFrame(data)
    
#     column_order = [
#         "对象", "飞行速度 (m/s)", "飞行方向 (rad)", 
#         "投放点 X", "投放点 Y", "投放点 Z", 
#         "起爆点 X", "起爆点 Y", "起爆点 Z"
#     ]
#     df = df[column_order]
    
#     try:
#         df.to_excel(filename, index=False, sheet_name=sheet_name)
#         print(f"结果已成功保存到 {filename}")
#     except Exception as e:
#         print(f"保存Excel失败: {e}")

# 任务五 - 全局协同策略
def save_final_results_to_excel(filename, global_strategy):
    """
    将全局协同策略保存到Excel文件中。
    global_strategy: 全局策略字典，格式为 {uav_id: {'speed': ..., 'angle': ..., 'grenades': [{'t_deploy': ..., 't_fuse': ..., 'target_missile': ...}]}}
    """
    data = []
    sorted_uav_ids = sorted(global_strategy.keys())

    for uav_id in sorted_uav_ids:
        strategy = global_strategy[uav_id]
        uav = UAV(uav_id)
        uav.set_flight_strategy(strategy['speed'], strategy['angle'])
        
        # 无人机飞行策略
        uav_data = {
            "对象": f"无人机 {uav_id}",
            "目标导弹": "N/A",
            "飞行速度 (m/s)": f"{strategy['speed']:.4f}",
            "飞行方向 (rad)": f"{strategy['angle']:.4f}",
            "投放时间 (s)": "N/A",
            "引信时间 (s)": "N/A",
            "投放点 X": "N/A", "投放点 Y": "N/A", "投放点 Z": "N/A",
            "起爆点 X": "N/A", "起爆点 Y": "N/A", "起爆点 Z": "N/A"
        }
        data.append(uav_data)
        
        # 该无人机投放的干扰弹策略
        for i, g_strat in enumerate(strategy['grenades']):
            t_deploy = g_strat['t_deploy']
            t_fuse = g_strat['t_fuse']
            target_missile = g_strat['target_missile']
            
            deploy_pos = uav.get_position(t_deploy)
            grenade = uav.deploy_grenade(t_deploy, t_fuse)
            detonate_pos = grenade.detonate_pos

            grenade_data = {
                "对象": f"  - 弹药 {i+1} (来自 {uav_id})",
                "目标导弹": target_missile,
                "飞行速度 (m/s)": "N/A",
                "飞行方向 (rad)": "N/A", 
                "投放时间 (s)": f"{t_deploy:.4f}",
                "引信时间 (s)": f"{t_fuse:.4f}",
                "投放点 X": f"{deploy_pos[0]:.4f}",
                "投放点 Y": f"{deploy_pos[1]:.4f}",
                "投放点 Z": f"{deploy_pos[2]:.4f}",
                "起爆点 X": f"{detonate_pos[0]:.4f}",
                "起爆点 Y": f"{detonate_pos[1]:.4f}",
                "起爆点 Z": f"{detonate_pos[2]:.4f}",
            }
            data.append(grenade_data)
            
    df = pd.DataFrame(data)
    
    # 调整列顺序
    column_order = [
        "对象", "目标导弹", "飞行速度 (m/s)", "飞行方向 (rad)", 
        "投放时间 (s)", "引信时间 (s)",
        "投放点 X", "投放点 Y", "投放点 Z", 
        "起爆点 X", "起爆点 Y", "起爆点 Z"
    ]
    df = df[column_order]
    
    try:
        df.to_excel(filename, index=False, sheet_name='result3')
        print(f"全局协同策略已成功保存到 {filename}")
    except Exception as e:
        print(f"保存Excel失败: {e}")

