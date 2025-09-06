# solve_problem_5.py
import numpy as np
import time
from optimizer import GlobalOptimizer
from config import *
from utils import save_final_results_to_excel

if __name__ == '__main__':
    # --- 步骤 0: 定义问题空间 ---
    ALL_UAV_IDS = list(UAVS_INITIAL.keys())
    ALL_MISSILE_IDS = list(MISSILES_INITIAL.keys())
    UAV_GRENADE_COUNTS = {u_id: 3 for u_id in ALL_UAV_IDS}

    print("="*70)
    print("      全局协同策略优化 (问题五)")
    print("  方法: 单一全局优化器 (differential_evolution)")
    print("="*70)

    # --- 步骤 1: 定义威胁权重 ---
    # 由于不再使用 threat_assessor，我们为所有导弹设置均等权重。
    print("\n--- 正在定义威胁权重 ---")
    num_missiles = len(ALL_MISSILE_IDS)
    threat_weights = {m_id: 1.0 / num_missiles for m_id in ALL_MISSILE_IDS}
    for m_id, weight in threat_weights.items():
        print(f"  - 导弹 {m_id} 的威胁权重 (均等): {weight:.3f}")
    print("--------------------------")

    # --- 步骤 2: 构建全局优化问题的边界 (Bounds) ---
    bounds = []
    sorted_uav_ids = sorted(ALL_UAV_IDS)
    
    for uav_id in sorted_uav_ids:
        num_grenades = UAV_GRENADE_COUNTS[uav_id]
        
        bounds.extend([
            (UAV_SPEED_MIN, UAV_SPEED_MAX), 
            (0, 2 * np.pi)
        ])
        
        for i in range(num_grenades):
            if i == 0:
                bounds.append((0.1, 30.0))
            else:
                bounds.append((GRENADE_INTERVAL, 15.0))
            
            bounds.append((0.1, 20.0))
            bounds.append((0.0, 1.0))

    # --- 步骤 3: 实例化并运行全局优化器 ---
    optimizer = GlobalOptimizer(
        uav_ids=ALL_UAV_IDS,
        missile_ids=ALL_MISSILE_IDS,
        threat_weights=threat_weights,
        uav_grenade_counts=UAV_GRENADE_COUNTS
    )

    D = len(bounds)
    print(f"\n全局优化问题维度: {D}")
    solver_options = {
        'popsize': 20 * D,
        'maxiter': 1500,
        'tol': 0.01, 
        'disp': True, 
        'workers': -1
    }
    
    print("--- 开始使用差分进化算法求解全局最优策略 ---")
    start_time = time.time()
    optimal_strategy, max_score = optimizer.solve(bounds, **solver_options)
    end_time = time.time() 
    print("--------------------------------------------")

    # --- 步骤 4: 展示和保存结果 ---
    print(f"\n优化完成，耗时: {end_time - start_time:.2f} 秒。")
    print(f"找到的最优策略的加权综合得分: {max_score:.4f}")
    print("="*70)

    print("\n--- 全局最优协同策略详情 ---")
    for uav_id, uav_strat in optimal_strategy.items():
        print(f"  UAV: {uav_id}")
        print(f"    飞行策略: speed = {uav_strat['speed']:.2f} m/s, angle = {np.degrees(uav_strat['angle']):.2f}°")
        for i, g in enumerate(uav_strat['grenades']):
            print(f"    - 弹药 {i+1}: t_deploy={g['t_deploy']:.2f}s, t_fuse={g['t_fuse']:.2f}s -> 目标: {g['target_missile']}")
    print("---------------------------------")

    save_final_results_to_excel('result_global_optimal.xlsx', optimal_strategy)