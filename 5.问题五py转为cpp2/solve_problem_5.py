# solve_problem_5.py
import numpy as np
import time
from optimizer import ObscurationOptimizer
from config import *
from utils import save_final_results_to_excel
from boundary_calculator import find_max_effective_deploy_time
# 导入新的分配器和威胁评估器
from task_allocator import assign_tasks_by_threat
from threat_assessor import assess_threat_weights

class Problem5SubOptimizer(ObscurationOptimizer):
    # ... (这个类完全不用修改) ...
    def _parse_decision_variables(self, dv):
        strategy = {}
        dv_index = 0
        sorted_uav_ids = sorted(self.uav_assignments.keys())
        for uav_id in sorted_uav_ids:
            num_grenades = self.uav_assignments[uav_id]
            speed, angle = dv[dv_index : dv_index + 2]
            dv_index += 2
            uav_strat = {'speed': speed, 'angle': angle, 'grenades': []}
            t_d1 = dv[dv_index]
            t_f1 = dv[dv_index + 1]
            uav_strat['grenades'].append({'t_deploy': t_d1, 't_fuse': t_f1})
            dv_index += 2
            last_td = t_d1
            for i in range(num_grenades - 1):
                delta_t = dv[dv_index]
                t_f = dv[dv_index + 1]
                current_td = last_td + delta_t
                uav_strat['grenades'].append({'t_deploy': current_td, 't_fuse': t_f})
                last_td = current_td
                dv_index += 2
            strategy[uav_id] = uav_strat
        return strategy


if __name__ == '__main__':
    # --- 步骤 0: 威胁评估 ---
    threat_weights = assess_threat_weights()
    
    # --- 步骤 1: 高层决策 (基于威胁权重) ---
    assignments = assign_tasks_by_threat(threat_weights)
    
    all_results = {}
    
    # --- 步骤 2: 低层决策 (与之前相同) ---
    for missile_id, uav_alloc in assignments.items():
        if not uav_alloc: continue # 跳过未分配到资源的导弹

        print("\n" + "="*60)
        print(f"开始为导弹 {missile_id} (威胁权重: {threat_weights[missile_id]:.2f}) 优化拦截策略...")
        print("分配的无人机及弹药: ", uav_alloc)
        print("="*60)

        # ... (动态构建边界的代码保持不变) ...
        bounds = []
        uav_ids_for_task = sorted(uav_alloc.keys())
        print("--- 正在计算 t_deploy 的有效边界 ---")
        for uav_id in uav_ids_for_task:
            num_grenades = uav_alloc[uav_id]
            t_max = find_max_effective_deploy_time(uav_id, missile_id)
            print(f"  {uav_id} 的 t_deploy 上边界建议为: {t_max:.2f} s")
            bounds.extend([(UAV_SPEED_MIN, UAV_SPEED_MAX), (0, 2 * np.pi)])
            bounds.extend([(0.1, t_max), (0.1, 20.0)])
            for _ in range(num_grenades - 1):
                bounds.extend([(GRENADE_INTERVAL, 10.0), (0.1, 20.0)])
        print("-----------------------------------")
        
        # 创建优化器，可以选择使用自适应DE或标准DE
        use_adaptive = True  # 设置为True使用自适应DE，False使用标准DE
        optimizer = Problem5SubOptimizer(missile_id=missile_id, uav_assignments=uav_alloc, use_adaptive_de=use_adaptive)
        D = len(bounds)
        print(f"该子问题的优化维度为: {D}")
        print(f"使用{'自适应' if use_adaptive else '标准'}差分进化算法")
        
        # 优化器选项
        if use_adaptive:
            # 自适应DE参数
            solver_options = {
                'popsize': min(15 * D, 200),  # 限制最大种群大小
                'maxiter': 800,  # 自适应DE通常收敛更快
                'tol': 0.01, 
                'disp': True
            }
        else:
            # 标准DE参数
            solver_options = {
                'popsize': 15 * D, 
                'maxiter': 1000, 
                'tol': 0.01, 
                'disp': True, 
                'workers': -1
            }
        
        start_time = time.time()
        optimal_strategy, max_time = optimizer.solve(bounds, **solver_options)
        end_time = time.time()
        
        print(f"\n对 {missile_id} 的优化完成，耗时: {end_time - start_time:.2f} 秒。")
        print(f"最大有效遮蔽时间: {max_time:.4f} s")
        
        all_results[missile_id] = {'strategy': optimal_strategy, 'time': max_time}

    # --- 步骤 3: 汇总、加权评分并保存结果 ---
    print("\n" + "="*60)
    print("所有优化任务完成，正在生成最终报告...")
    print("="*60)
    
    total_weighted_score = 0
    
    for missile_id, result in all_results.items():
        weight = threat_weights[missile_id]
        time = result['time']
        total_weighted_score += weight * time
        
        print(f"\n--- 导弹 {missile_id} 的最优策略 (遮蔽时间: {result['time']:.2f}s, 权重: {weight:.2f}) ---")
        # ... (打印策略详情的代码保持不变) ...
        for uav_id, uav_strat in result['strategy'].items():
            print(f"  UAV: {uav_id}")
            print(f"    飞行: speed={uav_strat['speed']:.2f}, angle={uav_strat['angle']:.2f}")
            for i, g in enumerate(uav_strat['grenades']):
                print(f"    弹药 {i+1}: t_deploy={g['t_deploy']:.2f}s, t_fuse={g['t_fuse']:.2f}s")
    
    print("\n" + "="*60)
    print(f"最终防御策略的加权综合得分: {total_weighted_score:.4f}")
    print("="*60)

    save_final_results_to_excel('result3.xlsx', all_results)