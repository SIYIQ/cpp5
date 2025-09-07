# optimizer.py
from scipy.optimize import differential_evolution
from core_objects import Missile, UAV, TargetCylinder
from config import TRUE_TARGET_SPECS
import numpy as np

class ObscurationOptimizer:
    def __init__(self, missile_id, uav_assignments):
        self.missile = Missile(missile_id)
        self.target = TargetCylinder(TRUE_TARGET_SPECS)
        self.uav_assignments = uav_assignments  # e.g., {'FY1': 3, 'FY2': 1}
        self.time_step = 0.1

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
            return 0

        sim_start_time = min(c.start_time for c in smoke_clouds)
        sim_end_time = max(c.end_time for c in smoke_clouds)

        obscured_time_points = set()
        for t in np.arange(sim_start_time, sim_end_time, self.time_step):
            is_obscured_at_t = any(cloud.check_obscuration_at_time(t, self.missile, self.target) for cloud in smoke_clouds)
            if is_obscured_at_t:
                obscured_time_points.add(round(t / self.time_step))
        
        total_time = len(obscured_time_points) * self.time_step
        return -total_time  # Return negative for maximization

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
