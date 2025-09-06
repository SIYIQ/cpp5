# optimizer.py
from scipy.optimize import differential_evolution
from core_objects import Missile, UAV, TargetCylinder
from config import *
import numpy as np
from geometry import check_collective_obscuration

class GlobalOptimizer:
    def __init__(self, uav_ids, missile_ids, threat_weights, uav_grenade_counts):
        self.uav_ids = sorted(list(uav_ids))
        self.missile_ids = sorted(list(missile_ids))
        self.threat_weights = threat_weights
        self.uav_grenade_counts = uav_grenade_counts
        
        # 创建所有会用到的对象
        self.missiles = {m_id: Missile(m_id) for m_id in self.missile_ids}
        self.uavs = {u_id: UAV(u_id) for u_id in self.uav_ids}
        self.target = TargetCylinder(TRUE_TARGET_SPECS)
        
        self.time_step = 0.1
        self.target_key_points = self.target.get_key_points()
        self.num_missiles = len(self.missile_ids)

    def _parse_decision_variables(self, dv):
        """
        将一维决策变量向量解析为结构化的全局策略。
        """
        strategy = {}
        dv_index = 0
        
        for uav_id in self.uav_ids:
            num_grenades = self.uav_grenade_counts[uav_id]
            
            # 1. 解析飞行策略
            speed, angle = dv[dv_index : dv_index + 2]
            dv_index += 2
            
            uav_strat = {'speed': speed, 'angle': angle, 'grenades': []}
            
            # 2. 解析每枚弹药的策略
            last_td = 0
            for i in range(num_grenades):
                # 问题五的变量结构
                if i == 0:
                    t_d = dv[dv_index]
                else:
                    # 后续弹药的投放时间是相对于前一枚的增量
                    delta_t = dv[dv_index]
                    t_d = last_td + delta_t
                
                t_f = dv[dv_index + 1]
                
                # 新增：解析该弹药的目标导弹
                # 用一个 0-1 之间的连续变量代表目标选择
                target_selector = dv[dv_index + 2] 
                # 将连续变量映射到离散的导弹索引
                target_missile_index = min(int(target_selector * self.num_missiles), self.num_missiles - 1)
                target_missile_id = self.missile_ids[target_missile_index]

                uav_strat['grenades'].append({
                    't_deploy': t_d, 
                    't_fuse': t_f,
                    'target_missile': target_missile_id
                })
                last_td = t_d
                dv_index += 3 # 每个弹药有3个决策变量: t_deploy/delta_t, t_fuse, target_selector
            
            strategy[uav_id] = uav_strat
            
        return strategy

    def objective_function(self, decision_variables):
        """
        计算给定全局策略下的加权总分。
        """
        try:
            strategy = self._parse_decision_variables(decision_variables)
        except (ValueError, IndexError):
            return 1e9 # 返回一个很大的惩罚值，表示这是一个无效的解

        all_smoke_clouds = []
        # 按无人机生成所有弹药并转化为烟云
        for uav_id, uav_strat in strategy.items():
            uav = self.uavs[uav_id]
            uav.set_flight_strategy(uav_strat['speed'], uav_strat['angle'])
            for g_strat in uav_strat['grenades']:
                grenade = uav.deploy_grenade(g_strat['t_deploy'], g_strat['t_fuse'])
                # 将烟云与它的目标导弹ID绑定
                cloud = grenade.generate_smoke_cloud()
                cloud.target_missile_id = g_strat['target_missile']
                all_smoke_clouds.append(cloud)

        if not all_smoke_clouds:
            return 1e9

        # 确定联合模拟的起止时间
        sim_start_time = min(c.start_time for c in all_smoke_clouds)
        sim_end_time = max(c.end_time for c in all_smoke_clouds)

        # 为每个导弹计算其被遮蔽的时间
        total_obscured_time_per_missile = {m_id: 0.0 for m_id in self.missile_ids}

        for t in np.arange(sim_start_time, sim_end_time, self.time_step):
            # 对每个导弹独立进行判断
            for missile_id in self.missile_ids:
                missile = self.missiles[missile_id]
                
                # 找出在当前时间 t，所有以该导弹为目标的有效烟云
                active_cloud_centers_for_missile = [
                    cloud.get_center(t) 
                    for cloud in all_smoke_clouds 
                    if cloud.target_missile_id == missile_id and cloud.get_center(t) is not None
                ]
                
                if not active_cloud_centers_for_missile:
                    continue

                # 判断这些烟云是否协同遮蔽了该导弹
                missile_pos = missile.get_position(t)
                if check_collective_obscuration(
                    missile_pos, 
                    active_cloud_centers_for_missile, 
                    self.target_key_points
                ):
                    total_obscured_time_per_missile[missile_id] += self.time_step
        
        # 计算加权总分
        total_weighted_score = 0
        for missile_id, obs_time in total_obscured_time_per_missile.items():
            total_weighted_score += self.threat_weights[missile_id] * obs_time
            
        # 优化器是最小化，所以返回分数的负数
        return -total_weighted_score

    def solve(self, bounds, **kwargs):
        """
        调用差分进化算法求解。
        """
        result = differential_evolution(
            self.objective_function,
            bounds,
            **kwargs
        )
        
        optimal_vars = result.x
        max_score = -result.fun
        
        # 重新解析最优变量以用于结果展示
        optimal_strategy = self._parse_decision_variables(optimal_vars)
        
        return optimal_strategy, max_score