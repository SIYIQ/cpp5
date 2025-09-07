import pyvista as pv
import numpy as np

# 假设这些文件存在且正确
from config import *
from core_objects import UAV, Missile, TargetCylinder
from geometry import check_full_obscuration

# --- 配置区 ---
# 1. 解决服务器无图形界面的问题
# 注意：新版本的 PyVista 推荐使用 pv.OFF_SCREEN = True
# 但为了兼容性，我们保留旧的方式，同时添加新方式的注释
import os
os.environ['PYVISTA_OFF_SCREEN'] = 'true'
pv.set_jupyter_backend(None)

# 2. --- 这是解决当前错误的关键 ---
# 允许 PyVista 绘制空的 mesh 对象，这对于创建占位符 actor 至关重要
pv.global_theme.allow_empty_mesh = True
# ------------------------------------

# 3. 这是你从问题三求解器得到的最优策略
OPTIMAL_STRATEGY_P3 = {
    'FY1': {
        'speed': 139.6956,
        'angle': 3.1338,
        'grenades': [
            {'t_deploy': 0.2281, 't_fuse': 3.7869},
            {'t_deploy': 3.3818, 't_fuse': 5.2772},
            {'t_deploy': 4.8330, 't_fuse': 5.9929}
        ]
    }
}

# --- 仿真参数 ---
SIMULATION_END_TIME = 65.0  # 模拟到导弹快要击中目标
SIMULATION_DT = 0.1       # 模拟的时间步长 (秒)
VIDEO_FRAMERATE = 30      # 输出视频的帧率
VIDEO_OUTPUT_FILENAME = "problem3_simulation_enhanced.mp4"


def generate_trajectory_data(strategy_dict, end_time, dt):
    """
    根据策略，计算出每一帧的仿真数据，并包含历史轨迹。
    """
    print("正在生成轨迹数据...")
    
    # 初始化对象
    uav = UAV('FY1')
    missile = Missile('M1')
    target = TargetCylinder(TRUE_TARGET_SPECS)
    uav.set_flight_strategy(strategy_dict['FY1']['speed'], strategy_dict['FY1']['angle'])

    # 创建所有烟幕云对象
    grenades = [uav.deploy_grenade(g['t_deploy'], g['t_fuse']) for g in strategy_dict['FY1']['grenades']]
    smoke_clouds = [g.generate_smoke_cloud() for g in grenades]
    
    # 用于存储历史轨迹
    uav_path = []
    missile_path = []

    trajectories = []
    time_axis = np.arange(0, end_time, dt)
    # 使用 enumerate 可以在需要时打印进度
    for frame_idx, t in enumerate(time_axis):
        frame_data = {'time': t}
        
        # 计算并记录当前位置和轨迹
        uav_pos = uav.get_position(t)
        uav_path.append(uav_pos)
        frame_data['uav_pos'] = uav_pos
        # 使用 copy() 确保每一帧的数据是独立的
        frame_data['uav_path'] = np.array(uav_path.copy())

        missile_pos = missile.get_position(t)
        missile_path.append(missile_pos)
        frame_data['missile_pos'] = missile_pos
        frame_data['missile_path'] = np.array(missile_path.copy())

        # 计算活动的云团
        active_clouds = []
        for i, cloud in enumerate(smoke_clouds):
            center = cloud.get_center(t)
            if center is not None:
                active_clouds.append({'center': center})
        frame_data['clouds'] = active_clouds
        
        # 计算遮蔽状态
        is_obscured = any(check_full_obscuration(missile_pos, c['center'], target) for c in active_clouds)
        frame_data['is_obscured'] = is_obscured
        
        trajectories.append(frame_data)
        
    print("轨迹数据生成完毕。")
    return trajectories


def create_simulation_video(trajectory_data, output_filename):
    """
    使用PyVista将轨迹数据渲染成具有增强视觉效果的视频。
    """
    print(f"正在创建仿真视频: {output_filename} ...")
    
    # 1. 设置绘图器
    plotter = pv.Plotter(off_screen=True, window_size=[1920, 1080])
    plotter.set_background('cornflowerblue')

    # 2. 创建场景和静态物体
    target_mesh = pv.Cylinder(center=target.bottom_center + np.array([0,0,target.height/2]),
                              direction=[0,0,1], radius=target.radius, height=target.height)
    target_actor = plotter.add_mesh(target_mesh, color='tan', show_edges=True, style='surface',
                                    specular=0.7, specular_power=15)
    
    ground_plane = pv.Plane(center=(10000, 0, 0), direction=(0, 0, 1),
                            i_size=40000, j_size=20000, i_resolution=40, j_resolution=20)
    plotter.add_mesh(ground_plane, style='wireframe', color='gray', opacity=0.5)
    plotter.add_axes(interactive=False, line_width=5)

    # 3. 创建所有动态对象的 "Actor"
    actors = {
        'uav': plotter.add_mesh(pv.Sphere(radius=150), color='#0077be', specular=0.5),
        'missile': plotter.add_mesh(pv.Cone(direction=(-1,0,0), height=400, radius=80), color='#333333', specular=0.8),
        # 因为我们设置了 allow_empty_mesh=True, 这里的空 PolyData 现在是允许的
        'uav_trail': plotter.add_mesh(pv.PolyData(), color='#0077be', line_width=4),
        'missile_trail': plotter.add_mesh(pv.PolyData(), color='#ff4500', line_width=6),
        'clouds': []
    }
    for _ in OPTIMAL_STRATEGY_P3['FY1']['grenades']:
        cloud_actor = plotter.add_mesh(pv.Sphere(radius=CLOUD_RADIUS), color='whitesmoke', opacity=0.4)
        cloud_actor.visibility = False
        actors['clouds'].append(cloud_actor)

    # 4. 设置光照和相机
    plotter.enable_shadows()
    light = pv.Light(position=(20000, 10000, 20000), light_type='scene light')
    plotter.add_light(light)
    
    focal_point = (15000, 200, 800)
    camera_position = (15000, -10000, 4000)
    plotter.camera.position = camera_position
    plotter.camera.focal_point = focal_point
    plotter.camera.zoom(1.2)

    # 5. 动画录制
    plotter.open_movie(output_filename, framerate=VIDEO_FRAMERATE)
    
    # 不再需要 plotter.show() 这一行，因为它在 off_screen 模式下没有作用且可能导致问题
    # plotter.show(auto_close=False, interactive=False)

    for frame_idx, frame in enumerate(trajectory_data):
        if frame_idx % VIDEO_FRAMERATE == 0:
             print(f"  ... 正在渲染第 {frame_idx}/{len(trajectory_data)} 帧 (时间: {frame['time']:.1f}s)")
        # 更新物体位置
        actors['uav'].position = frame['uav_pos']
        actors['missile'].position = frame['missile_pos']
        
        # 更新轨迹线 (通过修改 actor 引用的 mesh)
        if frame['uav_path'].shape[0] > 1:
            uav_line = pv.lines_from_points(frame['uav_path'])
            actors['uav_trail'].mapper.dataset.copy_from(uav_line)
            
        if frame['missile_path'].shape[0] > 1:
            missile_line = pv.lines_from_points(frame['missile_path'])
            actors['missile_trail'].mapper.dataset.copy_from(missile_line)

        # 更新云团、目标颜色、文本
        for i, cloud_actor in enumerate(actors['clouds']):
            if i < len(frame['clouds']):
                cloud_actor.visibility = True
                cloud_actor.position = frame['clouds'][i]['center']
            else:
                cloud_actor.visibility = False
                
        target_actor.prop.color = 'green' if frame['is_obscured'] else 'tan'

        # 使用 remove_actor 比 clear_actors 更安全，因为它只移除指定名称的 actor
        plotter.remove_actor(['time_text', 'status_text'], render=False)
        plotter.add_text(f"Time: {frame['time']:.1f} s", name='time_text', position='upper_left', font_size=20)
        status_text = "STATUS: OBSCURED" if frame['is_obscured'] else "STATUS: EXPOSED"
        status_color = "green" if frame['is_obscured'] else "red"
        plotter.add_text(status_text, name='status_text', position='upper_right', font_size=20, color=status_color)
        
        plotter.write_frame()

    plotter.close()
    print("视频生成完毕！")

if __name__ == '__main__':
    # 初始化全局对象 (仅为 TargetCylinder 需要)
    target = TargetCylinder(TRUE_TARGET_SPECS)
    
    # 1. 生成轨迹数据
    trajectories = generate_trajectory_data(OPTIMAL_STRATEGY_P3, SIMULATION_END_TIME, 1/VIDEO_FRAMERATE)
    
    # 2. 创建视频
    create_simulation_video(trajectories, VIDEO_OUTPUT_FILENAME)