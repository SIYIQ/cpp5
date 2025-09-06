# optimizer.py
from scipy.optimize import differential_evolution
from adaptive_de import adaptive_differential_evolution, BoundaryHandling
from core_objects import Missile, UAV, TargetCylinder
from config import TRUE_TARGET_SPECS
import numpy as np
from geometry import check_collective_obscuration
class ObscurationOptimizer:
    def __init__(self, missile_id, uav_assignments, use_adaptive_de=True):
        self.missile = Missile(missile_id)
        # TargetCylinder现在会自动生成完整的关键点集
        self.target = TargetCylinder(TRUE_TARGET_SPECS)
        self.uav_assignments = uav_assignments
        self.time_step = 0.1
        # 预先获取关键点，避免在循环中重复调用方法
        self.target_key_points = self.target.get_key_points()
        # 选择优化器类型
        self.use_adaptive_de = use_adaptive_de

    def _parse_decision_variables(self, decision_variables):
        # This method MUST be implemented by child classes for specific problems
        raise NotImplementedError

    def objective_function(self, decision_variables):
        strategies = self._parse_decision_variables(decision_variables)
        
        smoke_clouds = []
        try:
            for uav_id, uav_strat in strategies.items():
                uav = UAV(uav_id)
                uav.set_flight_strategy(uav_strat['speed'], uav_strat['angle'])
                for g_strat in uav_strat['grenades']:
                    grenade = uav.deploy_grenade(g_strat['t_deploy'], g_strat['t_fuse'])
                    smoke_clouds.append(grenade.generate_smoke_cloud())
        except (ValueError, KeyError):
            return 0 # Invalid strategy, return worst score

        if not smoke_clouds:
            return 0.0

        sim_start_time = min(c.start_time for c in smoke_clouds)
        sim_end_time = max(c.end_time for c in smoke_clouds)

        obscured_time_points = set()
        for t in np.arange(sim_start_time, sim_end_time, self.time_step):
            # a. 获取当前所有有效的云团中心
            active_cloud_centers = [
                cloud.get_center(t) for cloud in smoke_clouds if cloud.get_center(t) is not None
            ]
            
            if not active_cloud_centers:
                continue

            # b. 调用新的协同判断函数
            missile_pos = self.missile.get_position(t)
            if check_collective_obscuration(
                missile_pos, 
                active_cloud_centers, 
                self.target_key_points # 使用预先缓存的关键点
            ):
                obscured_time_points.add(round(t / self.time_step))
        
        total_time = len(obscured_time_points) * self.time_step
        return -total_time

    def solve(self, bounds, **kwargs):
        """
        求解优化问题
        
        Args:
            bounds: 优化边界
            **kwargs: 优化器参数
            
        Returns:
            tuple: (optimal_strategy, max_time)
        """
        if self.use_adaptive_de:
            # 使用自适应差分进化算法
            print("使用自适应差分进化算法 (JADE/SHADE)")
            
            # 设置自适应DE的默认参数
            adaptive_kwargs = {
                'maxiter': kwargs.get('maxiter', 1000),
                'tol': kwargs.get('tol', 0.01),
                'disp': kwargs.get('disp', True),
                'seed': kwargs.get('seed', None),
                'popsize': kwargs.get('popsize', None)
            }
            
            result = adaptive_differential_evolution(
                self.objective_function,
                bounds,
                **adaptive_kwargs
            )
            
            optimal_vars = result['x']
            max_time = -result['fun']
            
            if kwargs.get('disp', True):
                print(f"自适应DE优化完成:")
                print(f"  迭代次数: {result['nit']}")
                print(f"  收敛状态: {'成功' if result['success'] else '未收敛'}")
                print(f"  最终适应度: {result['fun']:.6f}")
        else:
            # 使用标准scipy差分进化算法
            print("使用标准差分进化算法 (scipy)")
            result = differential_evolution(
                self.objective_function,
                bounds,
                **kwargs
            )
            optimal_vars = result.x
            max_time = -result.fun
        
        # Re-create the optimal strategy for detailed output
        optimal_strategy = self._parse_decision_variables(optimal_vars)
        return optimal_strategy, max_time
