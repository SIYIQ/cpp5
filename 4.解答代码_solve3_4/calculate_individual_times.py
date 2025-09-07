# calculate_individual_times.py
# 计算每枚干扰弹各自的有效干扰时间

import numpy as np
from core_objects import UAV, Grenade, Missile, TargetCylinder, SmokeCloud
from config import TRUE_TARGET_SPECS
from geometry import check_collective_obscuration

def calculate_individual_grenade_times():
    """
    计算问题3中每枚干扰弹各自的有效干扰时间
    """
    print("="*60)
    print("计算每枚干扰弹各自的有效干扰时间")
    print("="*60)
    
    # 论文中的最优参数
    optimal_params = {
        'speed': 139.4411,      # m/s
        'angle': 3.1340,        # rad (179.53°)
        'grenades': [
            {'t_deploy': 0.3146, 't_fuse': 3.8931},  # 干扰弹1
            {'t_deploy': 2.9706, 't_fuse': 5.3031},  # 干扰弹2  
            {'t_deploy': 4.7841, 't_fuse': 5.9167}   # 干扰弹3
        ]
    }
    
    # 创建对象
    uav = UAV('FY1')
    missile = Missile('M1')
    target = TargetCylinder(TRUE_TARGET_SPECS)
    
    # 设置无人机策略
    uav.set_flight_strategy(optimal_params['speed'], optimal_params['angle'])
    
    print("各枚干扰弹的基本信息:")
    individual_times = []
    
    # 为每枚干扰弹单独计算有效干扰时间
    for i, g_strat in enumerate(optimal_params['grenades']):
        print(f"\n--- 干扰弹{i+1} ---")
        print(f"投放时间: {g_strat['t_deploy']:.4f}s")
        print(f"引信时长: {g_strat['t_fuse']:.4f}s")
        
        # 创建单个烟雾弹和烟雾云
        grenade = uav.deploy_grenade(g_strat['t_deploy'], g_strat['t_fuse'])
        smoke_cloud = grenade.generate_smoke_cloud()
        
        print(f"起爆时间: {smoke_cloud.start_time:.4f}s")
        print(f"消散时间: {smoke_cloud.end_time:.4f}s")
        # 计算投放点（从UAV获取）
        deploy_pos = uav.get_position(g_strat['t_deploy'])
        print(f"投放点: ({deploy_pos[0]:.4f}, {deploy_pos[1]:.4f}, {deploy_pos[2]:.4f})")
        print(f"起爆点: ({grenade.detonate_pos[0]:.4f}, {grenade.detonate_pos[1]:.4f}, {grenade.detonate_pos[2]:.4f})")
        
        # 计算该枚干扰弹的单独有效时间
        individual_time = 0.0
        dt = 0.1  # 时间步长
        
        for t in np.arange(smoke_cloud.start_time, smoke_cloud.end_time, dt):
            # 获取当前时刻烟雾云中心
            cloud_center = smoke_cloud.get_center(t)
            if cloud_center is not None:
                # 检查单个烟雾云的遮蔽效果
                missile_pos = missile.get_position(t)
                target_key_points = target.get_key_points()
                
                # 使用单个烟雾云进行遮蔽判断
                if check_collective_obscuration(missile_pos, [cloud_center], target_key_points):
                    individual_time += dt
        
        individual_times.append(individual_time)
        print(f"单独有效干扰时间: {individual_time:.4f}s")
    
    # 计算总时间验证
    print(f"\n" + "="*60)
    print("汇总结果:")
    print("="*60)
    total_time = 0.0
    dt = 0.1
    
    # 创建所有烟雾云
    all_smoke_clouds = []
    for g_strat in optimal_params['grenades']:
        grenade = uav.deploy_grenade(g_strat['t_deploy'], g_strat['t_fuse'])
        smoke_cloud = grenade.generate_smoke_cloud()
        all_smoke_clouds.append(smoke_cloud)
    
    # 计算协同总时间
    start_times = [cloud.start_time for cloud in all_smoke_clouds]
    end_times = [cloud.end_time for cloud in all_smoke_clouds]
    t_start = min(start_times)
    t_end = max(end_times)
    
    for t in np.arange(t_start, t_end, dt):
        active_clouds = []
        for cloud in all_smoke_clouds:
            center = cloud.get_center(t)
            if center is not None:
                active_clouds.append(center)
        
        if len(active_clouds) > 0:
            missile_pos = missile.get_position(t)
            target_key_points = target.get_key_points()
            if check_collective_obscuration(missile_pos, active_clouds, target_key_points):
                total_time += dt
    
    print("各枚干扰弹单独有效时间:")
    for i, time in enumerate(individual_times):
        print(f"  干扰弹{i+1}: {time:.4f}s")
    
    print(f"\n协同总有效时间: {total_time:.4f}s")
    print(f"论文中的总时间: 6.800s")
    
    print(f"\n" + "="*60)
    print("✅ 用于填写result1.xlsx的数据:")
    print("="*60)
    print("第0行 有效干扰时长 (s): {:.4f}".format(individual_times[0]))
    print("第1行 有效干扰时长 (s): {:.4f}".format(individual_times[1])) 
    print("第2行 有效干扰时长 (s): {:.4f}".format(individual_times[2]))
    print("="*60)

def calculate_individual_uav_times():
    """
    计算问题4中每架无人机各自的有效干扰时间
    """
    print("\n" + "="*60)
    print("计算每架无人机各自的有效干扰时间")
    print("="*60)
    
    # 论文中的最优参数
    optimal_strategies = {
        'FY1': {
            'speed': 125.0544,      # m/s
            'angle': 0.1193,        # rad (6.84°)
            'grenades': [{'t_deploy': 0.3501, 't_fuse': 0.1562}]
        },
        'FY2': {
            'speed': 139.4224,      # m/s  
            'angle': 3.9540,        # rad (226.60°)
            'grenades': [{'t_deploy': 7.5302, 't_fuse': 9.2195}]
        },
        'FY3': {
            'speed': 123.4114,      # m/s
            'angle': 1.6347,        # rad (93.67°)
            'grenades': [{'t_deploy': 21.2195, 't_fuse': 5.0828}]
        }
    }
    
    # 创建对象
    missile = Missile('M1')
    target = TargetCylinder(TRUE_TARGET_SPECS)
    
    print("各架无人机的基本信息:")
    individual_times = []
    
    # 为每架无人机单独计算有效干扰时间
    for uav_id, strategy in optimal_strategies.items():
        print(f"\n--- {uav_id} ---")
        print(f"飞行速度: {strategy['speed']:.4f} m/s")
        
        # 转换角度
        angle_degrees = np.degrees(strategy['angle'])
        if angle_degrees < 0:
            angle_degrees += 360
        angle_degrees = round(angle_degrees, 2)
        print(f"飞行角度: {angle_degrees:.2f}°")
        
        # 创建无人机
        uav = UAV(uav_id)
        uav.set_flight_strategy(strategy['speed'], strategy['angle'])
        
        g_strat = strategy['grenades'][0]
        print(f"投放时间: {g_strat['t_deploy']:.4f}s")
        print(f"引信时长: {g_strat['t_fuse']:.4f}s")
        
        # 创建单个烟雾弹和烟雾云
        grenade = uav.deploy_grenade(g_strat['t_deploy'], g_strat['t_fuse'])
        smoke_cloud = grenade.generate_smoke_cloud()
        
        print(f"起爆时间: {smoke_cloud.start_time:.4f}s")
        print(f"消散时间: {smoke_cloud.end_time:.4f}s")
        # 计算投放点（从UAV获取）
        deploy_pos = uav.get_position(g_strat['t_deploy'])
        print(f"投放点: ({deploy_pos[0]:.4f}, {deploy_pos[1]:.4f}, {deploy_pos[2]:.4f})")
        print(f"起爆点: ({grenade.detonate_pos[0]:.4f}, {grenade.detonate_pos[1]:.4f}, {grenade.detonate_pos[2]:.4f})")
        
        # 计算该架无人机的单独有效时间
        individual_time = 0.0
        dt = 0.1  # 时间步长
        
        for t in np.arange(smoke_cloud.start_time, smoke_cloud.end_time, dt):
            # 获取当前时刻烟雾云中心
            cloud_center = smoke_cloud.get_center(t)
            if cloud_center is not None:
                # 检查单个烟雾云的遮蔽效果
                missile_pos = missile.get_position(t)
                target_key_points = target.get_key_points()
                
                # 使用单个烟雾云进行遮蔽判断
                if check_collective_obscuration(missile_pos, [cloud_center], target_key_points):
                    individual_time += dt
        
        individual_times.append(individual_time)
        print(f"单独有效干扰时间: {individual_time:.4f}s")
    
    # 计算协同总时间验证
    print(f"\n协同效果验证:")
    total_time = 0.0
    dt = 0.1
    
    # 创建所有烟雾云
    all_smoke_clouds = []
    for uav_id, strategy in optimal_strategies.items():
        uav = UAV(uav_id)
        uav.set_flight_strategy(strategy['speed'], strategy['angle'])
        g_strat = strategy['grenades'][0]
        grenade = uav.deploy_grenade(g_strat['t_deploy'], g_strat['t_fuse'])
        smoke_cloud = grenade.generate_smoke_cloud()
        all_smoke_clouds.append(smoke_cloud)
    
    # 计算协同总时间
    start_times = [cloud.start_time for cloud in all_smoke_clouds]
    end_times = [cloud.end_time for cloud in all_smoke_clouds]
    t_start = min(start_times)
    t_end = max(end_times)
    
    for t in np.arange(t_start, t_end, dt):
        active_clouds = []
        for cloud in all_smoke_clouds:
            center = cloud.get_center(t)
            if center is not None:
                active_clouds.append(center)
        
        if len(active_clouds) > 0:
            missile_pos = missile.get_position(t)
            target_key_points = target.get_key_points()
            if check_collective_obscuration(missile_pos, active_clouds, target_key_points):
                total_time += dt
    
    print("各架无人机单独有效时间:")
    uav_list = ['FY1', 'FY2', 'FY3']
    for i, (uav_id, time) in enumerate(zip(uav_list, individual_times)):
        print(f"  {uav_id}: {time:.4f}s")
    
    print(f"\n协同总有效时间: {total_time:.4f}s")
    print(f"论文中的总时间: 12.600s")
    
    print(f"\n" + "="*60)
    print("✅ 用于填写result2.xlsx的数据:")
    print("="*60)
    print("第0行 有效干扰时长 (s): {:.4f}".format(individual_times[0]))  # FY1
    print("第1行 有效干扰时长 (s): {:.4f}".format(individual_times[1]))  # FY2
    print("第2行 有效干扰时长 (s): {:.4f}".format(individual_times[2]))  # FY3
    print("="*60)

if __name__ == '__main__':
    # 计算问题3的各枚干扰弹时间
    calculate_individual_grenade_times()
    
    # 计算问题4的各架无人机时间
    calculate_individual_uav_times()
