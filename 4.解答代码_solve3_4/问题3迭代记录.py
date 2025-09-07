# 问题3迭代记录.py
import numpy as np
import csv
import time
from optimizer import ObscurationOptimizer
from config import *
from scipy.optimize import differential_evolution

class Problem3ConvergenceTracker(ObscurationOptimizer):
    """
    针对问题三：FY1投放3枚干扰弹，对抗M1，并记录迭代过程。
    """
    def __init__(self, missile_id, uav_assignments):
        super().__init__(missile_id, uav_assignments)
        self.convergence_history = []
        
    def _parse_decision_variables(self, dv):
        """
        解析8维决策变量向量。
        dv = [speed, angle, t_d1, t_f1, delta_t2, t_f2, delta_t3, t_f3]
        """
        speed, angle, t_d1, t_f1, delta_t2, t_f2, delta_t3, t_f3 = dv
        
        # 从时间增量计算绝对投放时间
        t_d2 = t_d1 + delta_t2
        t_d3 = t_d2 + delta_t3
        
        strategy = {
            'FY1': {
                'speed': speed,
                'angle': angle,
                'grenades': [
                    {'t_deploy': t_d1, 't_fuse': t_f1},
                    {'t_deploy': t_d2, 't_fuse': t_f2},
                    {'t_deploy': t_d3, 't_fuse': t_f3}
                ]
            }
        }
        return strategy
    
    def objective_function_with_logging(self, decision_variables):
        """带记录功能的目标函数"""
        obj_value = super().objective_function(decision_variables)
        
        # 记录每次评估的结果（转换为正值）
        obscuration_time = -obj_value
        
        # 解析决策变量
        speed, angle, t_d1, t_f1, delta_t2, t_f2, delta_t3, t_f3 = decision_variables
        t_d2 = t_d1 + delta_t2
        t_d3 = t_d2 + delta_t3
        
        self.convergence_history.append({
            'iteration': len(self.convergence_history) + 1,
            'obscuration_time': obscuration_time,
            'speed': speed,
            'angle': angle,
            'angle_deg': np.degrees(angle),
            't_deploy1': t_d1,
            't_fuse1': t_f1,
            't_deploy2': t_d2,
            't_fuse2': t_f2,
            't_deploy3': t_d3,
            't_fuse3': t_f3,
            'delta_t2': delta_t2,
            'delta_t3': delta_t3
        })
        
        return obj_value
    
    def solve_with_tracking(self, bounds, **kwargs):
        """求解并记录迭代过程"""
        print("开始问题3的迭代跟踪求解...")
        
        result = differential_evolution(
            self.objective_function_with_logging,
            bounds,
            **kwargs
        )
        
        optimal_vars = result.x
        max_time = -result.fun
        optimal_strategy = self._parse_decision_variables(optimal_vars)
        
        return optimal_strategy, max_time
    
    def save_convergence_to_csv(self, filename):
        """保存收敛历史到CSV文件"""
        with open(filename, 'w', newline='', encoding='utf-8') as csvfile:
            fieldnames = [
                'iteration', 'obscuration_time', 'speed', 'angle', 'angle_deg',
                't_deploy1', 't_fuse1', 't_deploy2', 't_fuse2', 't_deploy3', 't_fuse3',
                'delta_t2', 'delta_t3'
            ]
            writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
            
            writer.writeheader()
            for record in self.convergence_history:
                writer.writerow(record)
        
        print(f"收敛历史已保存到: {filename}")
        print(f"总共记录了 {len(self.convergence_history)} 次函数评估")

if __name__ == '__main__':
    print("="*60)
    print("问题三迭代收敛过程记录")
    print("="*60)
    
    # 1. 定义决策变量的边界
    bounds = [
        (UAV_SPEED_MIN, UAV_SPEED_MAX),    # speed
        (0, 2 * np.pi),                    # angle
        (0.1, 25.0),                       # t_deploy1
        (0.1, 20.0),                       # t_fuse1
        (GRENADE_INTERVAL, 10.0),          # delta_t2 (min 1.0s)
        (0.1, 20.0),                       # t_fuse2
        (GRENADE_INTERVAL, 10.0),          # delta_t3 (min 1.0s)
        (0.1, 20.0)                        # t_fuse3
    ]
    
    print("决策变量边界设置:")
    print(f"  无人机飞行速度: [{UAV_SPEED_MIN}, {UAV_SPEED_MAX}] m/s")
    print(f"  无人机飞行方向: [0, {2*np.pi:.2f}] rad")
    print(f"  第1枚弹投放时间: [0.1, 25.0] s")
    print(f"  投弹间隔: [{GRENADE_INTERVAL}, 10.0] s")
    print(f"  引信时间: [0.1, 20.0] s")
    print("-"*60)

    # 2. 实例化优化器
    optimizer = Problem3ConvergenceTracker(missile_id='M1', uav_assignments={'FY1': 3})
    
    # 3. 设置求解器参数并运行优化
    solver_options = {
        'popsize': 80,        # 适中的种群规模
        'maxiter': 400,       # 适中的迭代次数
        'tol': 0.01,
        'disp': True,
        'workers': 1          # 单线程以确保记录顺序
    }
    
    print("优化算法参数:")
    print(f"  种群规模: {solver_options['popsize']}")
    print(f"  最大迭代次数: {solver_options['maxiter']}")
    print(f"  收敛容忍度: {solver_options['tol']}")
    print("-"*60)
    
    start_time = time.time()
    optimal_strategy_dict, max_time = optimizer.solve_with_tracking(bounds, **solver_options)
    end_time = time.time()
    
    print(f"\n优化完成，耗时: {end_time - start_time:.2f} 秒。")

    # 4. 打印结果
    print("\n" + "="*60)
    print("问题三 最优策略详情")
    print("="*60)
    print(f"最大有效遮蔽时间: {max_time:.4f} s")
    
    # 格式化打印策略
    uav_strat = optimal_strategy_dict['FY1']
    print(f"  无人机飞行速度: {uav_strat['speed']:.4f} m/s")
    print(f"  无人机飞行角度: {uav_strat['angle']:.4f} rad ({np.degrees(uav_strat['angle']):.2f}°)")
    for i, g in enumerate(uav_strat['grenades']):
        print(f"  - 干扰弹 {i+1}: 投放时间={g['t_deploy']:.4f}s, 引信时间={g['t_fuse']:.4f}s")

    print("="*60)

    # 5. 保存收敛历史
    optimizer.save_convergence_to_csv('问题3收敛历史.csv')
    
    print("\n问题3迭代记录完成！")
