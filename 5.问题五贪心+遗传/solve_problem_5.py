# solve_problem_5.py
import numpy as np
import time
from optimizer import GlobalOptimizer
from config import *
from utils import save_final_results_to_excel
# 导入威胁评估器
from threat_assessor import assess_threat_weights

if __name__ == '__main__':
    # --- 步骤 0: 定义问题空间 ---
    ALL_UAV_IDS = list(UAVS_INITIAL.keys())
    ALL_MISSILE_IDS = list(MISSILES_INITIAL.keys())
    UAV_GRENADE_COUNTS = {u_id: 3 for u_id in ALL_UAV_IDS}

    print("="*70)
    print("      全局协同策略优化 (问题五 - 修正版)")
    print("  方法: 全局优化器 (所有导弹考虑场上所有烟雾云)")
    print("="*70)

    # --- 步骤 1: 威胁评估 ---
    print("\n--- 正在进行威胁评估 ---")
    threat_weights = assess_threat_weights()
    print("--------------------------")

    # --- 步骤 2: 构建全局优化问题的边界 (Bounds) ---
    bounds = []
    sorted_uav_ids = sorted(ALL_UAV_IDS)
    
    for uav_id in sorted_uav_ids:
        num_grenades = UAV_GRENADE_COUNTS[uav_id]
        
        # 飞行策略边界
        bounds.extend([
            (UAV_SPEED_MIN, UAV_SPEED_MAX), 
            (0, 2 * np.pi)
        ])
        
        # 弹药策略边界
        for i in range(num_grenades):
            if i == 0:
                bounds.append((0.1, 30.0))  # t_deploy1
            else:
                bounds.append((GRENADE_INTERVAL, 15.0))  # delta_t
            
            bounds.append((0.1, 20.0))  # t_fuse
            bounds.append((0.0, 1.0))   # target_selector (新增)

    # --- 步骤 3: 实例化并运行全局优化器 ---
    optimizer = GlobalOptimizer(
        uav_ids=ALL_UAV_IDS,
        missile_ids=ALL_MISSILE_IDS,
        threat_weights=threat_weights,
        uav_grenade_counts=UAV_GRENADE_COUNTS
    )

    D = len(bounds)
    print(f"\n全局优化问题维度: {D}")
    
    # 调试版本：使用较小的参数进行快速测试
    solver_options = {
        'popsize': 10,        # 固定小种群，快速测试
        'maxiter': 100,       # 减少迭代次数
        'tol': 0.1,           # 放宽收敛条件
        'disp': True, 
        'workers': 1          # 单线程避免输出问题
    }
    print(f"使用调试参数: 种群={solver_options['popsize']}, 最大迭代={solver_options['maxiter']}")
    print("注意: 这是快速测试版本，如需高精度请调大参数")
    
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

    # 计算每个导弹的实际遮蔽时间用于分析
    print("\n--- 各导弹遮蔽时间分析 ---")
    all_smoke_clouds = []
    for uav_id, uav_strat in optimal_strategy.items():
        uav = UAV(uav_id)
        uav.set_flight_strategy(uav_strat['speed'], uav_strat['angle'])
        for g_strat in uav_strat['grenades']:
            grenade = uav.deploy_grenade(g_strat['t_deploy'], g_strat['t_fuse'])
            all_smoke_clouds.append(grenade.generate_smoke_cloud())

    # 分析每个导弹的实际遮蔽时间
    from geometry import check_collective_obscuration
    target = TargetCylinder(TRUE_TARGET_SPECS)
    target_key_points = target.get_key_points()
    time_step = 0.1
    
    if all_smoke_clouds:
        sim_start_time = min(c.start_time for c in all_smoke_clouds)
        sim_end_time = max(c.end_time for c in all_smoke_clouds)
        
        missile_obscured_times = {m_id: 0.0 for m_id in ALL_MISSILE_IDS}
        
        for t in np.arange(sim_start_time, sim_end_time, time_step):
            active_cloud_centers = [
                cloud.get_center(t) for cloud in all_smoke_clouds 
                if cloud.get_center(t) is not None
            ]
            
            if active_cloud_centers:
                for missile_id in ALL_MISSILE_IDS:
                    missile = Missile(missile_id)
                    missile_pos = missile.get_position(t)
                    
                    if check_collective_obscuration(missile_pos, active_cloud_centers, target_key_points):
                        missile_obscured_times[missile_id] += time_step
        
        for missile_id, obs_time in missile_obscured_times.items():
            weight = threat_weights[missile_id]
            print(f"  导弹 {missile_id}: 遮蔽时间 = {obs_time:.2f}s, 威胁权重 = {weight:.3f}, 加权得分 = {obs_time * weight:.3f}")
    
    print("-------------------------------")

    save_final_results_to_excel('result3.xlsx', optimal_strategy)