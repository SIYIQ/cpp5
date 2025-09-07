# solve_problem_1.py
import numpy as np
from core_objects import Missile, UAV, TargetCylinder
from config import TRUE_TARGET_SPECS

def solve_problem_1():
    # 1. --- 设置场景 ---
    missile_m1 = Missile('M1')
    uav_fy1 = UAV('FY1')
    true_target = TargetCylinder(TRUE_TARGET_SPECS)

    # 2. --- 定义无人机飞行策略 ---
    # 飞行方向：从FY1的起始位置飞向假目标(0,0,0)。
    # 飞行高度恒定，因此我们只考虑XY平面上的方向。
    uav_start_pos_xy = uav_fy1.start_pos[:2]
    decoy_pos_xy = np.array([0.0, 0.0])
    
    direction_vec_xy = decoy_pos_xy - uav_start_pos_xy
    
    # 飞行角度是该二维向量在XY平面上的角度。
    flight_angle = np.arctan2(direction_vec_xy[1], direction_vec_xy[0])
    
    flight_speed = 120.0  # 米/秒, 根据题目描述。
    
    uav_fy1.set_flight_strategy(speed=flight_speed, angle=flight_angle)

    # 3. --- 模拟烟幕弹投放 ---
    # 任务在 t=0 时指派。
    # 烟幕弹在 t=1.5秒 时投放。
    # 投弹3.6秒后爆炸。
    deploy_time = 1.5  # 秒
    fuse_time = 3.6    # 秒
    
    grenade = uav_fy1.deploy_grenade(deploy_time, fuse_time)

    # 4. --- 生成烟幕云 ---
    smoke_cloud = grenade.generate_smoke_cloud()

    # --- 为图2输出关键时刻坐标 ---
    print("\n" + "="*20 + " 数据导出：问题一关键时刻坐标 " + "="*20)
    # t=0s 时刻
    t0_uav_pos = uav_fy1.get_position(0)
    t0_m1_pos = missile_m1.get_position(0)
    print(f"t=0.0s (初始时刻):")
    print(f"  - FY1 位置: {np.round(t0_uav_pos, 2)}")
    print(f"  - M1 位置: {np.round(t0_m1_pos, 2)}")
    
    # t=1.5s 时刻
    t1_uav_pos = uav_fy1.get_position(deploy_time)
    t1_m1_pos = missile_m1.get_position(deploy_time)
    print(f"t=1.5s (投弹时刻):")
    print(f"  - FY1 位置 (投弹点): {np.round(t1_uav_pos, 2)}")
    print(f"  - M1 位置: {np.round(t1_m1_pos, 2)}")
    
    # t=5.1s 时刻
    t2_uav_pos = uav_fy1.get_position(smoke_cloud.start_time)
    t2_m1_pos = missile_m1.get_position(smoke_cloud.start_time)
    t2_cloud_pos = smoke_cloud.get_center(smoke_cloud.start_time)
    print(f"t=5.1s (起爆时刻):")
    print(f"  - FY1 位置: {np.round(t2_uav_pos, 2)}")
    print(f"  - M1 位置: {np.round(t2_m1_pos, 2)}")
    print(f"  - Smoke Cloud 中心: {np.round(t2_cloud_pos, 2)}")
    print("="*64 + "\n")


    # 5. --- 计算总遮蔽时间 ---
    total_obscured_time = 0.0
    time_step = 0.001  # 仿真时间步长（秒）。

    # 遍历烟幕云的整个有效持续时间。
    sim_start_time = smoke_cloud.start_time
    sim_end_time = smoke_cloud.end_time
    
    for t in np.arange(sim_start_time, sim_end_time, time_step):
        # 检查在当前时间步t，目标是否被遮蔽。
        is_obscured = smoke_cloud.check_obscuration_at_time(t, missile_m1, true_target)
        if is_obscured:
            total_obscured_time += time_step
            
    # 6. --- 打印结果 ---
    print(f"Problem 1 Solution:")
    print(f"UAV Speed: {flight_speed} m/s")
    print(f"UAV Direction Angle: {flight_angle:.4f} radians")
    print(f"Deploy Time: {deploy_time} s")
    print(f"Fuse Time: {fuse_time} s")
    print("-" * 30)
    print(f"Smoke Cloud Detonation Time: {smoke_cloud.start_time:.2f} s")
    print(f"Smoke Cloud Effective Duration: {smoke_cloud.end_time - smoke_cloud.start_time:.2f} s")
    print(f"Calculated Total Obscuration Time: {total_obscured_time:.2f} s")

if __name__ == '__main__':
    solve_problem_1()
