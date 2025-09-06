# solve_problem_3.py
import numpy as np
import time
from optimizer import ObscurationOptimizer
from config import *
from utils import save_results_to_excel

class Problem3Optimizer(ObscurationOptimizer):
    """
    针对问题三：FY1投放3枚干扰弹，对抗M1。
    """
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

if __name__ == '__main__':
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

    # 2. 实例化优化器
    # 告诉优化器，我们要用 FY1 去对付 M1，总共要规划3枚弹
    optimizer = Problem3Optimizer(missile_id='M1', uav_assignments={'FY1': 3})
    
    # 3. 设置求解器参数并运行优化
    # 对于更复杂的问题，增加种群规模和迭代次数是很有必要的
    solver_options = {
        'popsize': 150,       # 增加种群规模 (推荐 popsize * D ≈ 10 * 8 = 80 -> popsize=20-30)
        'maxiter': 1000,     # 增加最大迭代次数
        'tol': 0.01,
        'disp': True,
        'workers': -1        # 开启并行计算
    }
    
    print("="*50)
    print("开始求解问题三：FY1 vs M1 (3枚干扰弹)")
    print("="*50)
    
    start_time = time.time()
    optimal_strategy_dict, max_time = optimizer.solve(bounds, **solver_options)
    end_time = time.time()
    
    print(f"\n优化完成，耗时: {end_time - start_time:.2f} 秒。")

    # 4. 打印并保存结果
    print("\n" + "="*50)
    print("问题三 最优策略详情")
    print("="*50)
    print(f"最大有效遮蔽时间: {max_time:.4f} s")
    
    # 格式化打印策略
    uav_strat = optimal_strategy_dict['FY1']
    print(f"  无人机飞行速度: {uav_strat['speed']:.4f} m/s")
    print(f"  无人机飞行角度: {uav_strat['angle']:.4f} rad")
    for i, g in enumerate(uav_strat['grenades']):
        print(f"  - 干扰弹 {i+1}: 投放时间={g['t_deploy']:.4f}s, 引信时间={g['t_fuse']:.4f}s")

    print("="*50)

    # 5. 保存到Excel文件
    save_results_to_excel('result1.xlsx', uav_strat, uav_id='FY1')
