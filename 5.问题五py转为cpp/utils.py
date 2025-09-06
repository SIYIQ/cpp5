# utils.py
import pandas as pd
from core_objects import UAV

def save_global_strategy_to_excel(filename, global_strategy, sheet_name='global_strategy'):
    """
    将新的全局协同策略保存到指定的Excel文件中。
    
    Args:
        filename (str): 要保存的Excel文件名。
        global_strategy (dict): 由GlobalOptimizer生成的策略字典。
        sheet_name (str): Excel中的工作表名称。
    """
    data = []
    
    # 按无人机ID排序，保证输出顺序稳定
    sorted_uav_ids = sorted(global_strategy.keys())

    for uav_id in sorted_uav_ids:
        strategy = global_strategy[uav_id]
        uav = UAV(uav_id)
        uav.set_flight_strategy(strategy['speed'], strategy['angle'])
        
        # 1. 添加无人机自身的飞行策略行
        uav_data = {
            "对象": f"无人机 {uav_id}",
            "目标导弹": "N/A", # 无人机本身没有单一目标
            "飞行速度 (m/s)": f"{strategy['speed']:.4f}",
            "飞行方向 (rad)": f"{strategy['angle']:.4f}",
            "投放/起爆参数": "",
            "投放点 X": "", "投放点 Y": "", "投放点 Z": "",
            "起爆点 X": "", "起爆点 Y": "", "起爆点 Z": ""
        }
        data.append(uav_data)
        
        # 2. 添加该无人机投放的每枚弹药的信息行
        for i, g_strat in enumerate(strategy['grenades']):
            t_deploy = g_strat['t_deploy']
            t_fuse = g_strat['t_fuse']
            target_missile = g_strat['target_missile']
            
            # 计算投放点和起爆点
            deploy_pos = uav.get_position(t_deploy)
            # 注意: 这里为了获取起爆点，需要创建一个Grenade实例
            # 这会重复一些计算，但在工具函数中为了清晰是可接受的
            from core_objects import Grenade 
            grenade = Grenade(deploy_pos, uav.velocity_vec, t_deploy, t_fuse)
            detonate_pos = grenade.detonate_pos

            grenade_data = {
                "对象": f"  - 弹药 {i+1} (来自 {uav_id})",
                "目标导弹": target_missile,
                "飞行速度 (m/s)": "",
                "飞行方向 (rad)": "",
                "投放/起爆参数": f"t_deploy={t_deploy:.2f}s, t_fuse={t_fuse:.2f}s",
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
        "投放/起爆参数",
        "投放点 X", "投放点 Y", "投放点 Z", 
        "起爆点 X", "起爆点 Y", "起爆点 Z"
    ]
    df = df[column_order]
    
    try:
        df.to_excel(filename, index=False, sheet_name=sheet_name)
        print(f"\n全局策略已成功保存到: {filename}")
    except Exception as e:
        print(f"\n保存Excel失败: {e}")

# 为了让 solve_problem_5.py 能正确调用，我们保留旧的函数名，并让它指向新函数
save_final_results_to_excel = save_global_strategy_to_excel