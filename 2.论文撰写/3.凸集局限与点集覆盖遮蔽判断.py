import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import matplotlib.patches as mpatches
from matplotlib.lines import Line2D

# 设置matplotlib支持中文显示
plt.rcParams['font.sans-serif'] = ['SimHei']
plt.rcParams['axes.unicode_minus'] = False

# --- 模型参数（使用图1的坐标系统）---
TARGET_CENTER_BOTTOM = np.array([0.0, 100.0, 0.0])
TARGET_RADIUS = 30
TARGET_HEIGHT = 60.0
CLOUD_RADIUS = 30.0

# --- 绘图辅助函数 ---

def plot_cylinder(ax, center_bottom, radius, height, color='green', alpha=0.1, label=''):
    """在3D坐标系中绘制一个圆柱体"""
    z = np.linspace(0, height, 50)
    theta = np.linspace(0, 2 * np.pi, 50)
    theta_grid, z_grid = np.meshgrid(theta, z)
    x_grid = radius * np.cos(theta_grid) + center_bottom[0]
    y_grid = radius * np.sin(theta_grid) + center_bottom[1]
    z_grid = z_grid + center_bottom[2]
    ax.plot_surface(x_grid, y_grid, z_grid, alpha=alpha, color=color, label=label)

def plot_sphere(ax, center, radius, color='gray', alpha=0.3, label=''):
    """在3D坐标系中绘制一个球体"""
    u = np.linspace(0, 2 * np.pi, 100)
    v = np.linspace(0, np.pi, 100)
    x = radius * np.outer(np.cos(u), np.sin(v)) + center[0]
    y = radius * np.outer(np.sin(u), np.sin(v)) + center[1]
    z = radius * np.outer(np.ones(np.size(u)), np.cos(v)) + center[2]
    ax.plot_surface(x, y, z, color=color, alpha=alpha, label=label)

def plot_shadow_cone(ax, missile_pos, cloud_pos, cloud_radius, length=300, color='r', alpha=0.15):
    """绘制由导弹和烟雾云形成的阴影锥（参考图1的实现）"""
    # 锥顶和轴线
    v_cone = missile_pos
    cone_axis = cloud_pos - missile_pos
    dist_vc = np.linalg.norm(cone_axis)
    cone_axis_norm = cone_axis / dist_vc

    # 锥半角
    cone_half_angle = np.arcsin(cloud_radius / dist_vc)

    # 找到与轴线垂直的基向量 v1 和 v2，用于构建圆锥坐标系
    if np.abs(cone_axis_norm[0]) > 0.1:
        v1 = np.cross(cone_axis_norm, [1, 0, 0])
    else:
        v1 = np.cross(cone_axis_norm, [0, 1, 0])
    v1 /= np.linalg.norm(v1)
    v2 = np.cross(cone_axis_norm, v1)
    
    # 圆锥参数方程 (for surface)
    t_surf = np.linspace(0, length, 50)
    theta_surf = np.linspace(0, 2 * np.pi, 50)
    t_surf, theta_surf = np.meshgrid(t_surf, theta_surf)
    r_surf = t_surf * np.tan(cone_half_angle)
    x = v_cone[0] + t_surf * cone_axis_norm[0] + r_surf * (v1[0] * np.cos(theta_surf) + v2[0] * np.sin(theta_surf))
    y = v_cone[1] + t_surf * cone_axis_norm[1] + r_surf * (v1[1] * np.cos(theta_surf) + v2[1] * np.sin(theta_surf))
    z = v_cone[2] + t_surf * cone_axis_norm[2] + r_surf * (v1[2] * np.cos(theta_surf) + v2[2] * np.sin(theta_surf))
    
    # 绘制半透明的圆锥表面
    ax.plot_surface(x, y, z, color=color, alpha=alpha)

def generate_cylinder_key_points(center_bottom, radius, height, num_circ_samples=8, num_height_samples=3):
    """生成圆柱体关键点"""
    points = []
    
    # 底面圆心和顶面圆心
    points.append(center_bottom)
    points.append(center_bottom + np.array([0, 0, height]))
    
    # 底面和顶面圆周点
    for i in range(num_circ_samples):
        angle = 2 * np.pi * i / num_circ_samples
        offset_xy = np.array([radius * np.cos(angle), radius * np.sin(angle), 0.0])
        points.append(center_bottom + offset_xy)
        points.append(center_bottom + offset_xy + np.array([0, 0, height]))
    
    # 侧面母线点
    num_side_samples = 4
    for i in range(num_side_samples):
        angle = 2 * np.pi * i / num_side_samples
        offset_xy = np.array([radius * np.cos(angle), radius * np.sin(angle), 0.0])
        for j in range(1, num_height_samples):
            height_fraction = j / num_height_samples
            height_offset = np.array([0.0, 0.0, height * height_fraction])
            points.append(center_bottom + offset_xy + height_offset)
    
    return np.array(points)

def plot_key_points(ax, points, color='red', size=30):
    """绘制关键点"""
    ax.scatter(points[:, 0], points[:, 1], points[:, 2], 
               c=color, s=size, alpha=0.8, marker='o')

def setup_axis_uniform(ax):
    """统一设置坐标轴范围、刻度和视角"""
    ax.set_xlabel('X (m)')
    ax.set_ylabel('Y (m)')
    ax.set_zlabel('Z (m)')
    ax.set_xlim([-50, 400])
    ax.set_ylim([-50, 400])
    ax.set_zlim([0, 50])
    ax.set_xticks(np.arange(-50, 450, 50))
    ax.set_yticks(np.arange(-50, 400, 50))
    ax.set_zticks(np.arange(0, 300, 50))
    ax.view_init(elev=30, azim=-50)

# --- 主绘图函数 ---

def create_comparison_figure():
    """创建三个子图的对比图"""
    fig = plt.figure(figsize=(18, 6))
    
    # 导弹位置（使用图1的坐标系统）
    missile_pos = np.array([300, 190, 70])  # 参考图1的坐标系统
    
    # 子图1：凸集方法成功 - 单烟雾完全遮蔽
    ax1 = fig.add_subplot(131, projection='3d')
    cloud_center1 = np.array([72, 233, 4])  # 烟雾云位置你可以自己调整
    
    # 绘制目标圆柱体
    plot_cylinder(ax1, TARGET_CENTER_BOTTOM, TARGET_RADIUS, TARGET_HEIGHT, 
                 color='green', alpha=0.7)
    # 绘制烟雾云
    plot_sphere(ax1, cloud_center1, CLOUD_RADIUS, color='gray', alpha=0.6)
    # 绘制阴影锥（使用图1的方法）
    plot_shadow_cone(ax1, missile_pos, cloud_center1, CLOUD_RADIUS, 
                    length=500, color='r', alpha=0.15)
    # 绘制导弹位置
    ax1.scatter(*missile_pos, color='red', s=80)
    
    # 设置统一的坐标轴和视角
    setup_axis_uniform(ax1)
    
    # 子图2：凸集方法局限 - 两烟雾各遮蔽一半但凸集判断失败
    ax2 = fig.add_subplot(132, projection='3d')
    cloud_center2a = np.array([72, 233, -30])  # 遮蔽一部分，你可以自己调整
    cloud_center2b = np.array([72, 233, 25])  # 遮蔽另一部分，你可以自己调整
    CLOUD_RADIUS1=27
    # 绘制目标圆柱体
    plot_cylinder(ax2, TARGET_CENTER_BOTTOM, TARGET_RADIUS, TARGET_HEIGHT, 
                 color='green', alpha=0.7)
    # 绘制两个烟雾云
    plot_sphere(ax2, cloud_center2a, CLOUD_RADIUS1, color='gray', alpha=0.6)
    plot_sphere(ax2, cloud_center2b, CLOUD_RADIUS1, color='lightblue', alpha=0.6)
    # 绘制两个阴影锥
    plot_shadow_cone(ax2, missile_pos, cloud_center2a, CLOUD_RADIUS, 
                    length=500, color='r', alpha=0.15)
    plot_shadow_cone(ax2, missile_pos, cloud_center2b, CLOUD_RADIUS, 
                    length=500, color='b', alpha=0.15)
    # 绘制导弹位置
    ax2.scatter(*missile_pos, color='red', s=80)
    
    # 设置统一的坐标轴和视角
    setup_axis_uniform(ax2)
    
    # 子图3：关键点覆盖方法成功 - 显示关键点
    ax3 = fig.add_subplot(133, projection='3d')
    
    # 绘制目标圆柱体
    plot_cylinder(ax3, TARGET_CENTER_BOTTOM, TARGET_RADIUS, TARGET_HEIGHT, 
                 color='green', alpha=0.7)
    # 生成并绘制关键点
    key_points = generate_cylinder_key_points(TARGET_CENTER_BOTTOM, TARGET_RADIUS, TARGET_HEIGHT)
    plot_key_points(ax3, key_points, color='purple', size=20)
    # 绘制两个烟雾云
    plot_sphere(ax3, cloud_center2a, CLOUD_RADIUS, color='gray', alpha=0.6)
    plot_sphere(ax3, cloud_center2b, CLOUD_RADIUS, color='lightblue', alpha=0.6)
    # 绘制两个阴影锥
    plot_shadow_cone(ax3, missile_pos, cloud_center2a, CLOUD_RADIUS, 
                    length=500, color='r', alpha=0.15)
    plot_shadow_cone(ax3, missile_pos, cloud_center2b, CLOUD_RADIUS, 
                    length=500, color='b', alpha=0.15)
    # 绘制导弹位置
    ax3.scatter(*missile_pos, color='red', s=80)
    
    # 设置统一的坐标轴和视角
    setup_axis_uniform(ax3)
    
    # 添加图例（参考图2的风格）
    legend_elements = [
        mpatches.Patch(color='green', alpha=0.7, label='真实目标'),
        mpatches.Patch(color='gray', alpha=0.6, label='烟雾云1'),
        mpatches.Patch(color='lightblue', alpha=0.6, label='烟雾云2'),
        Line2D([0], [0], marker='o', color='w', markerfacecolor='red', markersize=10, label='来袭导弹'),
        mpatches.Patch(color='red', alpha=0.15, label='阴影锥1'),
        mpatches.Patch(color='blue', alpha=0.15, label='阴影锥2')
    ]
    
    fig.legend(handles=legend_elements, loc='upper center', ncol=6, 
               bbox_to_anchor=(0.5, 0.99), fontsize=10)
    
    # 添加可移动的标题（参考图2的做法）
    fig.text(0.17, 0.88, '(a) 凸包遮蔽:单烟雾遮蔽判断成功', ha='center', va='center', fontsize=12)
    fig.text(0.50, 0.88, '(b) 凸包遮蔽:多烟雾判断的局限性', ha='center', va='center', fontsize=12)
    fig.text(0.83, 0.88, '(c) 关键点覆盖:多烟雾判断的成功', ha='center', va='center', fontsize=12)
    
    plt.tight_layout(rect=[0, 0.05, 1, 0.93])
    
    # 保存图片
    plt.savefig('3.凸集局限与关键点覆盖对比图.png', dpi=300, bbox_inches='tight')

if __name__ == "__main__":
    create_comparison_figure()
