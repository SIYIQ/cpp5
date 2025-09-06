import matplotlib.pyplot as plt
import matplotlib.colors as mcolors
import numpy as np
import time

from core_objects import UAV, Missile, TargetCylinder
from config import *

# --- 1. 粘贴或加载你的最优策略 ---
# 请将这里替换为你 solve_problem_3.py 运行得到的最优策略
strategy_q3 = {
    'speed': 139.4411 , 
    'angle': 3.1340,
    'grenades': [
        {'t_deploy': 0.3146, 't_fuse': 3.8931},
        {'t_deploy': 2.9706, 't_fuse': 5.3031},
        {'t_deploy': 4.7841, 't_fuse': 5.9167}
    ]
}

def calculate_and_plot_timeline():
    """
    主函数，负责计算遮蔽区间并绘制时序图。
    """
    print("正在进行详细仿真以计算每个干扰弹的精确遮蔽时间窗口...")
    start_sim_time = time.time()

    # --- a. 初始化仿真环境 ---
    uav = UAV('FY1')
    uav.set_flight_strategy(strategy_q3['speed'], strategy_q3['angle'])
    missile = Missile('M1')
    target = TargetCylinder(TRUE_TARGET_SPECS)
    target_key_points = target.get_key_points() # 使用协同遮蔽的判断逻辑

    # 创建所有烟幕云团对象
    smoke_clouds = [
        uav.deploy_grenade(g['t_deploy'], g['t_fuse']).generate_smoke_cloud()
        for g in strategy_q3['grenades']
    ]
    num_grenades = len(smoke_clouds)

    # --- b. 进行高精度仿真，记录每个云团的遮蔽区间 ---
    sim_start_time_range = min(c.start_time for c in smoke_clouds)
    sim_end_time_range = max(c.end_time for c in smoke_clouds)
    time_step = 0.1
    time_axis = np.arange(sim_start_time_range, sim_end_time_range, time_step)
    
    obscuration_intervals = {i: [] for i in range(num_grenades)}
    obscuration_starts = {i: None for i in range(num_grenades)}
    all_intervals_for_merge = []

    for t in time_axis:
        missile_pos = missile.get_position(t)
        for i, cloud in enumerate(smoke_clouds):
            cloud_center = cloud.get_center(t)
            is_obscuring = False
            if cloud_center is not None:
                # 使用 check_collective_obscuration 但只传入一个云团
                # 这等价于单云团对全表面的判断
                from geometry import check_collective_obscuration
                is_obscuring = check_collective_obscuration(missile_pos, [cloud_center], target_key_points)

            # 状态机：检测遮蔽开始和结束的瞬间
            if is_obscuring and obscuration_starts[i] is None:
                obscuration_starts[i] = t
            elif not is_obscuring and obscuration_starts[i] is not None:
                start = obscuration_starts[i]
                duration = t - start
                obscuration_intervals[i].append((start, duration))
                all_intervals_for_merge.append((start, t)) # 记录 start, end
                obscuration_starts[i] = None

    # 处理仿真结束时仍在遮蔽的情况
    for i, start_t in obscuration_starts.items():
        if start_t is not None:
            duration = sim_end_time_range - start_t
            obscuration_intervals[i].append((start_t, duration))
            all_intervals_for_merge.append((start_t, sim_end_time_range))

    end_sim_time = time.time()
    print(f"仿真完成，耗时 {end_sim_time - start_sim_time:.2f} 秒。")

    # --- c. 合并所有区间以计算总遮蔽时间 ---
    if not all_intervals_for_merge:
        total_intervals = []
    else:
        # 按开始时间排序
        all_intervals_for_merge.sort(key=lambda x: x[0])
        
        merged = [all_intervals_for_merge[0]]
        for current_start, current_end in all_intervals_for_merge[1:]:
            last_start, last_end = merged[-1]
            if current_start <= last_end: # 区间重叠
                merged[-1] = (last_start, max(last_end, current_end))
            else:
                merged.append((current_start, current_end))
        
        # 转换为 (start, duration) 格式
        total_intervals = [(start, end - start) for start, end in merged]

    total_duration = sum(duration for start, duration in total_intervals)

    # --- d. 绘制Gantt图 ---
    fig, ax = plt.subplots(figsize=(14, 6))
    
    # 定义Y轴标签和位置
    labels = [f'Grenade {i+1}' for i in range(num_grenades)] + ['Total Obscuration']
    y_pos = np.arange(len(labels))
    
    # 绘制每个干扰弹的遮蔽条
    colors = plt.cm.viridis(np.linspace(0, 1, num_grenades))
    for i in range(num_grenades):
        if obscuration_intervals[i]:
            ax.broken_barh(obscuration_intervals[i], (y_pos[i] - 0.4, 0.8), facecolors=colors[i], alpha=0.8)

    # 绘制总遮蔽条
    if total_intervals:
        ax.broken_barh(total_intervals, (y_pos[-1] - 0.4, 0.8), facecolors='crimson')

    ax.set_yticks(y_pos)
    ax.set_yticklabels(labels)
    ax.invert_yaxis()  # 让 "Grenade 1" 在最上面

    ax.set_xlabel('Time (seconds since mission start)')
    ax.set_ylabel('Interference Source')
    ax.set_title(f'Obscuration Timeline for Problem 3\nTotal Effective Duration: {total_duration:.2f} s', fontsize=16)
    
    ax.grid(axis='x', linestyle='--', alpha=0.6)
    plt.tight_layout()
    plt.savefig('problem3_timeline_chart.png')
    plt.show()


if __name__ == '__main__':
    # 确保 geometry.py 在同一个目录下
    calculate_and_plot_timeline()