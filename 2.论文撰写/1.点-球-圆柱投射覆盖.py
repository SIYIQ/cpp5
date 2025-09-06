import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import os
from matplotlib.patches import Patch
from matplotlib.lines import Line2D

# 设置matplotlib支持中文显示
plt.rcParams['font.sans-serif'] = ['SimHei']
plt.rcParams['axes.unicode_minus'] = False

# --- 1. 从 config.py 获取模型参数 ---
# 真实目标圆柱体
TARGET_CENTER_BOTTOM = np.array([0.0, 200.0, 0.0])
TARGET_RADIUS = 40.0
TARGET_HEIGHT = 90.0

# 烟雾云球体
CLOUD_RADIUS = 40.0


# --- 2. 绘图辅助函数 ---

def plot_cylinder(ax, center_bottom, radius, height, color='green', alpha=0.1, label=''):
    """在3D坐标系中绘制一个圆柱体"""
    z = np.linspace(0, height, 50)
    theta = np.linspace(0, 2 * np.pi, 50)
    theta_grid, z_grid = np.meshgrid(theta, z)
    x_grid = radius * np.cos(theta_grid) + center_bottom[0]
    y_grid = radius * np.sin(theta_grid) + center_bottom[1]
    z_grid = z_grid + center_bottom[2]

    # 只绘制圆柱体表面，不再绘制轮廓线
    ax.plot_surface(x_grid, y_grid, z_grid,
                    alpha=alpha,
                    color=color,
                    label=label)


def plot_sphere(ax, center, radius, color='gray', alpha=0.3, label=''):
    """在3D坐标系中绘制一个球体"""
    u = np.linspace(0, 2 * np.pi, 100)
    v = np.linspace(0, np.pi, 100)
    x = radius * np.outer(np.cos(u), np.sin(v)) + center[0]
    y = radius * np.outer(np.sin(u), np.sin(v)) + center[1]
    z = radius * np.outer(np.ones(np.size(u)), np.cos(v)) + center[2]

    # 1. 控制球体表面填充
    ax.plot_surface(x, y, z,
                    color=color,      # --> 表面颜色 (如 'gray')
                    alpha=alpha,      # --> 表面透明度
                    label=label)

    # 2. 控制球体外部轮廓线 (经纬线) (通过 rstride 和 cstride 减少线条密度)
    ax.plot_wireframe(x, y, z, color='k', lw=0.8, alpha=0.7, rstride=10, cstride=10)


def plot_shadow_cone(ax, missile_pos, cloud_pos, cloud_radius, length=500, color='r', alpha=0.15):
    """绘制由导弹和烟雾云形成的阴影锥"""
    # 锥顶和轴线
    v_cone = missile_pos
    cone_axis = cloud_pos - missile_pos
    dist_vc = np.linalg.norm(cone_axis)
    cone_axis_norm = cone_axis / dist_vc

    # 锥半角
    cone_half_angle = np.arcsin(cloud_radius / dist_vc)

    # --- 修正：恢复被误删的基向量计算 ---
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
    
    # 只绘制半透明的圆锥表面，不再绘制紫色线条
    ax.plot_surface(x, y, z, color=color, alpha=alpha)


# --- 3. 场景生成函数 ---

def create_scenario_plot(ax, missile_pos, cloud_pos, title):
    """在一个给定的子图(ax)上创建一个完整的场景"""

    # 绘制目标圆柱体 (高饱和度，轻微透明)
    plot_cylinder(ax, TARGET_CENTER_BOTTOM, TARGET_RADIUS, TARGET_HEIGHT, color='green', alpha=0.7)

    # 绘制烟雾云 (比目标更透明一些)
    plot_sphere(ax, cloud_pos, CLOUD_RADIUS, color='gray', alpha=0.6)

    # 绘制导弹
    ax.scatter(*missile_pos, color='red', s=80)

    # 绘制假目标
    ax.scatter([0], [400], [0], color='blue', marker='x', s=100)

    # 绘制阴影锥
    plot_shadow_cone(ax, missile_pos, cloud_pos, CLOUD_RADIUS, length=700)

    # 设置坐标轴
    ax.set_xlabel('X (m)')
    ax.set_ylabel('Y (m)')
    ax.set_zlabel('Z (m)')
    ax.set_title(title, fontsize=14)

    # 设置视角和坐标范围，确保所有物体可见
    ax.set_xlim([-20, 400])
    ax.set_ylim([100, 500])
    ax.set_zlim([0, 400])
    # 调整相机视角 (方位角 azim)，使其看起来“朝外”
    ax.view_init(elev=20, azim=-50)


# --- 4. 主程序：在一张图中生成三种情况的子图 ---

if __name__ == "__main__":
    # 创建一个1x3的子图布局
    fig, (ax1, ax2, ax3) = plt.subplots(1, 3, figsize=(24, 8), subplot_kw={'projection': '3d'})
    
    # --- 定义三种情况的位置参数 ---
    # 为了便于对比，我们固定导弹位置，仅移动烟雾云
    missile_pos = np.array([300.0, 300.0, 300.0])

    # 情况一：完全覆盖 (烟雾云与目标中心精准对齐)
    cloud_pos_1 = np.array([66.0, 360.0, 102.0])
    create_scenario_plot(ax1, missile_pos, cloud_pos_1, 'Case1:烟雾完全遮蔽真目标')

    # 情况二：覆盖一部分 (烟雾云在Y轴有轻微偏移)
    cloud_pos_2 = np.array([66.0, 405.0, 102.0])
    create_scenario_plot(ax2, missile_pos, cloud_pos_2, 'Case2:烟雾部分遮蔽真目标')

    # 情况三：完全未覆盖 (烟雾云在Y轴有较大偏移)
    cloud_pos_3 = np.array([60.0, 440.0, 102.0])
    create_scenario_plot(ax3, missile_pos, cloud_pos_3, 'Case3:烟雾未能遮蔽真目标')

    # --- 创建并添加统一的图例 ---
    legend_elements = [
        Patch(facecolor='green', alpha=0.7, label='真实目标'),
        Patch(facecolor='gray', alpha=0.6, label='烟雾云团'),
        Line2D([0], [0], marker='o', color='w', label='来袭导弹',
               markerfacecolor='red', markersize=10),
        Line2D([0], [0], marker='x', color='blue', linestyle='None',
               markersize=8, markeredgewidth=2, label='假目标')
    ]
    fig.legend(handles=legend_elements, loc='upper center', ncol=4, fontsize=12)
    
    # 调整布局并保存
    plt.tight_layout(rect=[0, 0, 1, 0.91]) # 为图例留出空间
    
    # --- 保存图像 ---
    # 获取脚本所在的目录
    script_dir = os.path.dirname(os.path.abspath(__file__))
    # 新的文件名，与脚本的编号"1"对应
    output_filename = '1.遮蔽效果对比图(三种情况).png'
    # 构建完整的输出路径
    full_output_path = os.path.join(script_dir, output_filename)
    
    plt.savefig(full_output_path, dpi=300)
    print(f"场景对比图像已保存至 '{full_output_path}'")
    plt.close()