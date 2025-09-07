# 问题2迭代记录.py
from optimizer import ObscurationOptimizer
from config import *
import numpy as np
import csv
from scipy.optimize import differential_evolution

class Problem2ConvergenceTracker(ObscurationOptimizer):
    def __init__(self, missile_id, uav_assignments):
        super().__init__(missile_id, uav_assignments)
        self.convergence_history = []
        self.iteration_count = 0
        # 确保使用正确的时间步长
        self.time_step = 0.1
        
    def _parse_decision_variables(self, dv):
        # dv = [speed, angle, t_deploy, t_fuse]
        speed, angle, t_d, t_f = dv
        return {
            'FY1': {
                'speed': speed,
                'angle': angle,
                'grenades': [{'t_deploy': t_d, 't_fuse': t_f}]
            }
        }
    
    def objective_function_with_logging(self, decision_variables):
        """带记录功能的目标函数"""
        # 确保时间步长正确设置
        self.time_step = 0.05  # 使用与原始solve_problem_2相同的精度
        obj_value = super().objective_function(decision_variables)
        
        # 记录每次评估的结果（转换为正值）
        obscuration_time = -obj_value
        
        # 添加调试信息
        if len(self.convergence_history) < 5:  # 只打印前5次
            print(f"评估 {len(self.convergence_history)+1}: 参数={decision_variables}, 遮蔽时间={obscuration_time:.4f}s")
        
        self.convergence_history.append({
            'iteration': len(self.convergence_history) + 1,
            'obscuration_time': obscuration_time,
            'speed': decision_variables[0],
            'angle': decision_variables[1],
            'angle_deg': np.degrees(decision_variables[1]),
            't_deploy': decision_variables[2],
            't_fuse': decision_variables[3]
        })
        
        return obj_value
    
    def solve_with_tracking(self, bounds, **kwargs):
        """求解并记录迭代过程"""
        print("开始问题2的迭代跟踪求解...")
        
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
            fieldnames = ['iteration', 'obscuration_time', 'speed', 'angle', 'angle_deg', 't_deploy', 't_fuse']
            writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
            
            writer.writeheader()
            for record in self.convergence_history:
                writer.writerow(record)
        
        print(f"收敛历史已保存到: {filename}")
        print(f"总共记录了 {len(self.convergence_history)} 次函数评估")

if __name__ == '__main__':
    print("="*60)
    print("问题二迭代收敛过程记录")
    print("="*60)
    
    # Define bounds for [speed, angle, t_deploy, t_fuse]
    bounds = [
        (UAV_SPEED_MIN, UAV_SPEED_MAX),  # (70, 140) m/s
        (0, 2 * np.pi),                 # (0, 6.28) rad
        (0.1, 15),                      # 投放时间 (秒)
        (0.1, 20.0)                     # 引信时间 (秒)
    ]
    
    print("决策变量边界设置:")
    print(f"  无人机飞行速度: [{UAV_SPEED_MIN}, {UAV_SPEED_MAX}] m/s")
    print(f"  无人机飞行方向: [0, {2*np.pi:.2f}] rad")
    print(f"  烟雾弹投放时间: [0.1, 15] s")
    print(f"  烟雾弹引信时长: [0.1, 20.0] s")
    print("-"*60)

    optimizer = Problem2ConvergenceTracker(missile_id='M1', uav_assignments={'FY1': 1})
    
    print("开始差分进化优化求解...")
    solver_options = {
        'popsize': 60,      # 适中的种群规模
        'maxiter': 300,     # 适中的迭代次数
        'tol': 0.01, 
        'disp': True, 
        'workers': 1        # 单线程以确保记录顺序
    }
    
    print(f"优化算法参数:")
    print(f"  种群规模: {solver_options['popsize']}")
    print(f"  最大迭代次数: {solver_options['maxiter']}")
    print(f"  收敛容忍度: {solver_options['tol']}")
    print("-"*60)
    
    optimal_strategy, max_time = optimizer.solve_with_tracking(bounds, **solver_options)
    
    print("\n" + "="*60)
    print("问题二最优解详情")
    print("="*60)
    print(f"最大有效遮蔽时间: {max_time:.4f} s")
    
    # 提取最优策略参数
    fy1_strategy = optimal_strategy['FY1']
    optimal_speed = fy1_strategy['speed']
    optimal_angle = fy1_strategy['angle']
    optimal_t_deploy = fy1_strategy['grenades'][0]['t_deploy']
    optimal_t_fuse = fy1_strategy['grenades'][0]['t_fuse']
    
    print("最优决策变量:")
    print(f"  无人机FY1飞行速度: {optimal_speed:.4f} m/s")
    print(f"  无人机FY1飞行方向: {optimal_angle:.4f} rad ({np.degrees(optimal_angle):.2f}°)")
    print(f"  烟雾弹投放时间: {optimal_t_deploy:.4f} s")
    print(f"  烟雾弹引信时长: {optimal_t_fuse:.4f} s")
    print("="*60)
    
    # 保存收敛历史
    optimizer.save_convergence_to_csv('问题2收敛历史.csv')
    
    print("\n问题2迭代记录完成！")
