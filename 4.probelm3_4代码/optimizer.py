# optimizer.py
from scipy.optimize import differential_evolution
from core_objects import Missile, UAV, TargetCylinder
from config import TRUE_TARGET_SPECS
import numpy as np
from geometry import check_collective_obscuration
from config import TRUE_TARGET_SPECS
import numpy as np
class ObscurationOptimizer:
    def __init__(self, missile_id, uav_assignments):
        self.missile = Missile(missile_id)
        # TargetCylinder现在会自动生成完整的关键点集
        self.target = TargetCylinder(TRUE_TARGET_SPECS)
        self.uav_assignments = uav_assignments
        self.time_step = 0.1
        # 预先获取关键点，避免在循环中重复调用方法
        self.target_key_points = self.target.get_key_points()

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
