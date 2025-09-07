# core_objects.py
import numpy as np
from config import *
from geometry import check_full_obscuration
from scipy.integrate import solve_ivp
class TargetCylinder:
    def __init__(self, specs):
        self.radius = specs['radius']
        self.height = specs['height']
        self.bottom_center = specs['center_bottom']
        self.top_center = self.bottom_center + np.array([0.0, 0.0, self.height])

class Missile:
    def __init__(self, missile_id):
        self.id = missile_id
        specs = MISSILES_INITIAL[missile_id]
        self.start_pos = specs['pos']
        self.speed = specs['speed']
        direction_vec = specs['target'] - self.start_pos
        self.unit_vec = direction_vec / np.linalg.norm(direction_vec)

    def get_position(self, t):
        return self.start_pos + self.unit_vec * self.speed * t

class SmokeCloud:
    def __init__(self, detonate_pos, detonate_time):
        self.detonate_pos = detonate_pos
        self.start_time = detonate_time
        self.end_time = detonate_time + CLOUD_DURATION

    def get_center(self, t):
        if not (self.start_time <= t < self.end_time):
            return None
        t_since_detonate = t - self.start_time
        return self.detonate_pos - np.array([0.0, 0.0, CLOUD_SINK_SPEED * t_since_detonate])
    
    def check_obscuration_at_time(self, t, missile, target):
        missile_pos = missile.get_position(t)
        cloud_center = self.get_center(t)
        if cloud_center is None:
            return False
        return check_full_obscuration(missile_pos, cloud_center, target)

def grenade_motion_ode(t, y, mass, drag_factor):
    """
    定义烟幕弹运动的常微分方程组。
    y 是一个6维状态向量: [x, y, z, vx, vy, vz]
    """
    # y[0:3] 是位置, y[3:6] 是速度
    velocity = y[3:6]
    
    # 计算加速度
    gravity_accel = np.array([0.0, 0.0, -G])
    
    speed = np.linalg.norm(velocity)
    if speed > 1e-6:
        drag_accel = -(drag_factor / mass) * speed * velocity
    else:
        drag_accel = np.array([0.0, 0.0, 0.0])
        
    total_accel = gravity_accel + drag_accel
    
    # 返回状态向量的导数 [d(pos)/dt, d(vel)/dt] = [vel, accel]
    return np.concatenate((velocity, total_accel))

class Grenade:
    def __init__(self, deploy_pos, deploy_vel, deploy_time, fuse_time):
        self.deploy_time = deploy_time
        self.fuse_time = fuse_time
        self.detonate_time = self.deploy_time + self.fuse_time
        
        # 使用专业的ODE求解器计算起爆点
        self.detonate_pos = self._solve_trajectory_scipy(deploy_pos, deploy_vel)

    def _solve_trajectory_scipy(self, deploy_pos, deploy_vel):
        """
        使用 scipy.integrate.solve_ivp 求解弹道。
        """
        # 初始状态向量 [x, y, z, vx, vy, vz]
        y0 = np.concatenate((deploy_pos, deploy_vel))
        
        # 求解时间范围 [0, fuse_time]
        t_span = [0, self.fuse_time]
        
        # 我们只关心最终时刻的结果，所以设置 t_eval=[self.fuse_time]
        solution = solve_ivp(
            fun=grenade_motion_ode,      # 微分方程
            t_span=t_span,               # 时间范围
            y0=y0,                       # 初始条件
            method='RK45',               # 求解器方法 (常用且高效)
            t_eval=[self.fuse_time],     # 指定输出时间的点
            args=(GRENADE_MASS, GRENADE_DRAG_FACTOR) # 传递给 fun 的额外参数
        )
        
        # 提取最终时刻的位置
        final_state = solution.y[:, -1]
        final_position = final_state[0:3]
        
        return final_position
    
    def generate_smoke_cloud(self):
        return SmokeCloud(self.detonate_pos, self.detonate_time)
    
class UAV:
    def __init__(self, uav_id):
        self.id = uav_id
        self.start_pos = UAVS_INITIAL[uav_id]['pos']
        self.speed = None
        self.angle = None
        self.velocity_vec = None

    def set_flight_strategy(self, speed, angle):
        self.speed = speed
        self.angle = angle
        self.velocity_vec = speed * np.array([np.cos(angle), np.sin(angle), 0.0])
    
    def get_position(self, t):
        if self.velocity_vec is None:
            raise ValueError("UAV flight strategy has not been set.")
        return self.start_pos + self.velocity_vec * t
    
    def deploy_grenade(self, deploy_time, fuse_time):
        if self.velocity_vec is None:
            raise ValueError("UAV flight strategy has not been set.")
        deploy_pos = self.get_position(deploy_time)
        return Grenade(deploy_pos, self.velocity_vec, deploy_time, fuse_time)
