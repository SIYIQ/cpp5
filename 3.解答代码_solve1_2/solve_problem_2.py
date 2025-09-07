# solve_problem_2.py
from optimizer import ObscurationOptimizer
from config import *
import numpy as np

class Problem2Optimizer(ObscurationOptimizer):
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

if __name__ == '__main__':
    print("="*60)
    print("问题二：无人机FY1单弹优化策略求解")
    print("="*60)
    
    # Define bounds for [speed, angle, t_deploy, t_fuse]
    bounds = [
        (UAV_SPEED_MIN, UAV_SPEED_MAX),  # (70, 140) m/s
        (0, 2 * np.pi),                 # (0, 6.28) rad
        (0.1, 15),                    # 投放时间 (秒) - 通过boundary_calculator计算得出
        (0.1, 20.0)                     # 引信时间 (秒)
    ]
    
    print("决策变量边界设置:")
    print(f"  无人机飞行速度: [{UAV_SPEED_MIN}, {UAV_SPEED_MAX}] m/s")
    print(f"  无人机飞行方向: [0, {2*np.pi:.2f}] rad")
    print(f"  烟雾弹投放时间: [0.1, 13.9] s (通过边界分析确定)")
    print(f"  烟雾弹引信时长: [0.1, 20.0] s")
    print("-"*60)

    optimizer = Problem2Optimizer(missile_id='M1', uav_assignments={'FY1': 1})
    
    print("开始差分进化优化求解...")
    print("优化算法参数:")
    solver_options = {'popsize': 300, 'maxiter': 6000, 'tol': 0.01, 'disp': True, 'workers': -1}
    print(f"  种群规模: {solver_options['popsize']}")
    print(f"  最大迭代次数: {solver_options['maxiter']}")
    print(f"  收敛容忍度: {solver_options['tol']}")
    print(f"  并行计算: {'开启' if solver_options['workers'] == -1 else '关闭'}")
    print("-"*60)
    
    optimal_strategy, max_time = optimizer.solve(bounds, **solver_options)
    
    print("\n" + "="*60)
    print("问题二最优解详情")
    print("="*60)
    print(f"最大有效遮蔽时间: {max_time:.4f} s")
    print()
    
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
    print(f"  烟雾弹起爆时间: {optimal_t_deploy + optimal_t_fuse:.4f} s")
    print()
    
    # 计算一些关键信息用于分析
    from core_objects import UAV, Missile
    uav_fy1 = UAV('FY1')
    uav_fy1.set_flight_strategy(optimal_speed, optimal_angle)
    
    deploy_pos = uav_fy1.get_position(optimal_t_deploy)
    grenade = uav_fy1.deploy_grenade(optimal_t_deploy, optimal_t_fuse)
    detonate_pos = grenade.detonate_pos
    
    print("关键位置信息:")
    print(f"  投放点坐标: ({deploy_pos[0]:.2f}, {deploy_pos[1]:.2f}, {deploy_pos[2]:.2f}) m")
    print(f"  起爆点坐标: ({detonate_pos[0]:.2f}, {detonate_pos[1]:.2f}, {detonate_pos[2]:.2f}) m")
    
    # 计算导弹在起爆时刻的位置
    missile_m1 = Missile('M1')
    missile_pos_at_detonate = missile_m1.get_position(optimal_t_deploy + optimal_t_fuse)
    print(f"  导弹M1在起爆时刻位置: ({missile_pos_at_detonate[0]:.2f}, {missile_pos_at_detonate[1]:.2f}, {missile_pos_at_detonate[2]:.2f}) m")
    
    print("="*60)
