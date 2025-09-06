# solve_problem_4.py
import numpy as np
import time
from optimizer import ObscurationOptimizer
from config import *
# 使用新的、更通用的Excel保存函数
from utils import save_multi_uav_results_to_excel
# 我们可以复用边界计算器来为每架无人机确定边界
from boundary_calculator import find_max_effective_deploy_time

class Problem4Optimizer(ObscurationOptimizer):
    """
    针对问题四：FY1, FY2, FY3 各投放1枚干扰弹，对抗M1。
    """
    def _parse_decision_variables(self, dv):
        """
        解析12维决策变量向量。
        dv = [s1, a1, td1, tf1, s2, a2, td2, tf2, s3, a3, td3, tf3]
        """
        s1, a1, td1, tf1 = dv[0:4]
        s2, a2, td2, tf2 = dv[4:8]
        s3, a3, td3, tf3 = dv[8:12]
        
        strategy = {
            'FY1': {
                'speed': s1, 'angle': a1,
                'grenades': [{'t_deploy': td1, 't_fuse': tf1}]
            },
            'FY2': {
                'speed': s2, 'angle': a2,
                'grenades': [{'t_deploy': td2, 't_fuse': tf2}]
            },
            'FY3': {
                'speed': s3, 'angle': a3,
                'grenades': [{'t_deploy': td3, 't_fuse': tf3}]
            }
        }
        return strategy

if __name__ == '__main__':
    UAV_IDS = ['FY1', 'FY2', 'FY3']
    MISSILE_ID = 'M1'

    # 1. 为每架无人机动态计算 t_deploy 的上边界
    print("--- 正在为各无人机计算 t_deploy 的有效边界 ---")
    t_deploy_max_bounds = {}
    for uav_id in UAV_IDS:
        # 注意: boundary_calculator 内部的最优飞行角度假设可能需要调整
        # 对于FY2, FY3，最优角度不一定是 pi，但 pi 作为一个保守的“向前冲”策略来定界是合理的。
        t_max = find_max_effective_deploy_time(uav_id, MISSILE_ID)
        t_deploy_max_bounds[uav_id] = t_max
        print(f"  {uav_id} 的 t_deploy 上边界建议为: {t_max:.2f} s")
    print("-------------------------------------------------")

    # 2. 定义12个决策变量的总边界
    bounds_fy1 = [
        (UAV_SPEED_MIN, UAV_SPEED_MAX), (0, 2 * np.pi),
        (0.1, t_deploy_max_bounds['FY1']), (0.1, 20.0)
    ]
    bounds_fy2 = [
        (UAV_SPEED_MIN, UAV_SPEED_MAX), (0, 2 * np.pi),
        (0.1, t_deploy_max_bounds['FY2']), (0.1, 20.0)
    ]
    bounds_fy3 = [
        (UAV_SPEED_MIN, UAV_SPEED_MAX), (0, 2 * np.pi),
        (0.1, t_deploy_max_bounds['FY3']), (0.1, 20.0)
    ]
    bounds = bounds_fy1 + bounds_fy2 + bounds_fy3

    # 3. 实例化优化器
    optimizer = Problem4Optimizer(missile_id=MISSILE_ID, uav_assignments={'FY1': 1, 'FY2': 1, 'FY3': 1})
    
    # 4. 设置求解器参数并运行优化
    # 12维问题需要更大的种群和更多的迭代
    D = 12 # 维度
    solver_options = {
        'popsize': 15 * D,   # 经验法则: popsize 至少为 10*D
        'maxiter': 2000,     # 增加迭代次数
        'tol': 0.01,
        'disp': True,
        'workers': -1
    }
    
    print("\n" + "="*50)
    print("开始求解问题四：FY1, FY2, FY3 vs M1 (各1枚)")
    print("="*50)
    
    start_time = time.time()
    optimal_strategy_dict, max_time = optimizer.solve(bounds, **solver_options)
    end_time = time.time()
    
    print(f"\n优化完成，耗时: {end_time - start_time:.2f} 秒。")

    # 5. 打印并保存结果
    print("\n" + "="*50)
    print("问题四 最优策略详情")
    print("="*50)
    print(f"最大总有效遮蔽时间: {max_time:.4f} s")
    
    for uav_id, uav_strat in optimal_strategy_dict.items():
        print(f"\n--- {uav_id} 策略 ---")
        print(f"  飞行速度: {uav_strat['speed']:.4f} m/s")
        print(f"  飞行角度: {uav_strat['angle']:.4f} rad")
        g = uav_strat['grenades'][0]
        print(f"  投放时间: {g['t_deploy']:.4f}s, 引信时间: {g['t_fuse']:.4f}s")

    print("="*50)

    # 6. 保存到Excel文件
    save_multi_uav_results_to_excel('result2.xlsx', optimal_strategy_dict, sheet_name='result2')