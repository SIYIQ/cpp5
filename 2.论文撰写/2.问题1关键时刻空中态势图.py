import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import matplotlib.patches as mpatches
from matplotlib.lines import Line2D

# --- 绘图辅助函数 ---

def plot_cylinder(ax, bottom_center, radius, height, color='green', alpha=0.7):
    """绘制一个底部中心在指定位置的圆柱体"""
    z = np.linspace(0, height, 50)
    theta = np.linspace(0, 2 * np.pi, 50)
    theta_grid, z_grid = np.meshgrid(theta, z)
    x_grid = radius * np.cos(theta_grid) + bottom_center[0]
    y_grid = radius * np.sin(theta_grid) + bottom_center[1]
    z_grid = z_grid + bottom_center[2]
    ax.plot_surface(x_grid, y_grid, z_grid, alpha=alpha, color=color, linewidth=0)

def plot_sphere(ax, center, radius, color='gray', alpha=0.6):
    """绘制一个球体"""
    u = np.linspace(0, 2 * np.pi, 100)
    v = np.linspace(0, np.pi, 100)
    x = radius * np.outer(np.cos(u), np.sin(v)) + center[0]
    y = radius * np.outer(np.sin(u), np.sin(v)) + center[1]
    z = radius * np.outer(np.ones(np.size(u)), np.cos(v)) + center[2]
    ax.plot_surface(x, y, z, color=color, alpha=alpha, linewidth=0)

def plot_shadow_cone(ax, missile_pos, cloud_pos, cloud_radius, length=500, color='r', alpha=0.15):
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
    ax.plot_surface(x, y, z, color='orange', alpha=alpha)

def setup_ax(ax):
    """设置子图的坐标轴和视角"""
    ax.set_xlabel('X (m)')
    ax.set_ylabel('Y (m)')
    ax.set_zlabel('Z (m)')
    # 隐藏X轴的刻度数值
    ax.set_xticklabels([])
    # 视角设置：Y轴朝里，突出XZ平面
    ax.view_init(elev=15, azim=-100)
    ax.grid(True)


def main():
    """主函数，生成并保存图像"""
    plt.rcParams['font.sans-serif'] = ['SimHei']
    plt.rcParams['axes.unicode_minus'] = False

    fig = plt.figure(figsize=(18, 5))

    # --- 场景通用参数 ---
    true_target_bottom = np.array([17000, 200, 0])
    true_target_radius = 210.0
    true_target_height = 300.0
    # 假目标在UAV飞行方向（-X）很远的地方，这里仅为示意
    decoy_target_pos = np.array([17000, -300, 0]) 
    cloud_radius = 300.0

    # --- 从终端获取的关键坐标 ---
    coords = {
        't0': { 'uav': np.array([17800., 0., 1800.]), 'm1': np.array([20000., 0., 2000.]) },
        't1.5': { 'uav': np.array([17620., 0., 1800.]), 'm1': np.array([19552.23, 0., 1955.22]) },
        't5.1': { 'uav': np.array([17188., 0., 1800.]), 'm1': np.array([18477.59, 0., 1847.76]), 'cloud': np.array([17700.4, 0., 900.53]) }
    }

    # --- 子图1: t = 0s (初始时刻) ---
    ax1 = fig.add_subplot(1, 3, 1, projection='3d')
    plot_cylinder(ax1, true_target_bottom, true_target_radius, true_target_height)
    ax1.scatter(*coords['t0']['m1'], color='red', s=50)
    ax1.scatter(*coords['t0']['uav'], color='blue', marker='^', s=80)
    # 在无人机上显示未爆烟幕弹
    ax1.scatter(*coords['t0']['uav'], color='lime', s=50, ec='black')
    ax1.scatter(*decoy_target_pos, color='purple', marker='x', s=100)
    # 添加导弹轨迹箭头
    m1_pos_t0 = coords['t0']['m1']
    direction_vec_m1 = decoy_target_pos - m1_pos_t0
    arrow_vec_m1 = direction_vec_m1 / np.linalg.norm(direction_vec_m1) * 3520 # 箭头长度加倍
    ax1.quiver(m1_pos_t0[0], m1_pos_t0[1], m1_pos_t0[2],
               arrow_vec_m1[0], arrow_vec_m1[1], arrow_vec_m1[2],
               color='purple', linewidth=1, arrow_length_ratio=0.1)
    # 添加无人机轨迹箭头
    uav_pos_t0 = coords['t0']['uav']
    ax1.quiver(uav_pos_t0[0], uav_pos_t0[1], uav_pos_t0[2], 
               -500, 0, 0, # 箭头长度500m
               color='blue', linewidth=1, arrow_length_ratio=0.3)
    setup_ax(ax1)
    
    # --- 子图2: t = 1.5s (投弹时刻) ---
    ax2 = fig.add_subplot(1, 3, 2, projection='3d')
    plot_cylinder(ax2, true_target_bottom, true_target_radius, true_target_height)
    ax2.scatter(*coords['t1.5']['m1'], color='red', s=50)
    ax2.scatter(*coords['t1.5']['uav'], color='blue', marker='^', s=80)
    # 在无人机位置绘制一个绿色点代表刚投放的烟幕弹
    ax2.scatter(*coords['t1.5']['uav'], color='lime', s=50, ec='black')
    ax2.scatter(*decoy_target_pos, color='purple', marker='x', s=100)
    # 添加导弹轨迹箭头
    m1_pos_t1_5 = coords['t1.5']['m1']
    direction_vec_m1 = decoy_target_pos - m1_pos_t1_5
    arrow_vec_m1 = direction_vec_m1 / np.linalg.norm(direction_vec_m1) * 3100 # 箭头长度加倍
    ax2.quiver(m1_pos_t1_5[0], m1_pos_t1_5[1], m1_pos_t1_5[2],
               arrow_vec_m1[0], arrow_vec_m1[1], arrow_vec_m1[2],
               color='purple', linewidth=1, arrow_length_ratio=0.1)
    # 添加无人机轨迹箭头
    uav_pos_t1_5 = coords['t1.5']['uav']
    ax2.quiver(uav_pos_t1_5[0], uav_pos_t1_5[1], uav_pos_t1_5[2], 
               -500, 0, 0, 
               color='blue', linewidth=1, arrow_length_ratio=0.3)
    setup_ax(ax2)

    # --- 子图3: t = 5.1s (起爆时刻) ---
    ax3 = fig.add_subplot(1, 3, 3, projection='3d')
    plot_cylinder(ax3, true_target_bottom, true_target_radius, true_target_height)
    plot_sphere(ax3, coords['t5.1']['cloud'], cloud_radius)
    ax3.scatter(*coords['t5.1']['m1'], color='red', s=50)
    ax3.scatter(*coords['t5.1']['uav'], color='blue', marker='^', s=80)
    ax3.scatter(*decoy_target_pos, color='purple', marker='x', s=100)
    # 添加导弹轨迹箭头
    m1_pos_t5_1 = coords['t5.1']['m1']
    direction_vec_m1 = decoy_target_pos - m1_pos_t5_1
    arrow_vec_m1 = direction_vec_m1 / np.linalg.norm(direction_vec_m1) * 2250 # 箭头长度加倍
    ax3.quiver(m1_pos_t5_1[0], m1_pos_t5_1[1], m1_pos_t5_1[2],
               arrow_vec_m1[0], arrow_vec_m1[1], arrow_vec_m1[2],
               color='purple', linewidth=1, arrow_length_ratio=0.1)
    # 添加无人机轨迹箭头
    uav_pos_t5_1 = coords['t5.1']['uav']
    ax3.quiver(uav_pos_t5_1[0], uav_pos_t5_1[1], uav_pos_t5_1[2], 
               -500, 0, 0, 
               color='blue', linewidth=1, arrow_length_ratio=0.3)
    # 绘制阴影锥
    plot_shadow_cone(ax3, coords['t5.1']['m1'], coords['t5.1']['cloud'], cloud_radius, length=2200)
    setup_ax(ax3)
    
    # --- 统一调整所有子图的坐标范围，使其一致 ---
    all_pos = np.array(list(coords['t0'].values()) + list(coords['t1.5'].values()) + list(coords['t5.1'].values()))
    x_min, y_min, z_min = all_pos.min(axis=0)
    x_max, y_max, z_max = all_pos.max(axis=0)
    ax_center = np.mean(all_pos, axis=0)
    ax_range = np.max([x_max-x_min, y_max-y_min, z_max-z_min]) * 0.6
    
    for ax in [ax1, ax2, ax3]:
        ax.set_xlim(ax_center[0] - ax_range, ax_center[0] + ax_range)
        ax.set_ylim(ax_center[1] - ax_range, ax_center[1] + ax_range)
        ax.set_zlim(ax_center[2] - ax_range, ax_center[2] + ax_range)

    # --- 创建统一图例 ---
    legend_elements = [
        mpatches.Patch(color='green', alpha=0.7, label='真实目标'),
        Line2D([0], [0], marker='o', color='w', label='导弹 M1', markerfacecolor='red', markersize=10),
        Line2D([0], [0], marker='^', color='w', label='无人机 FY1', markerfacecolor='blue', markersize=10),
        Line2D([0], [0], marker='o', color='w', label='烟幕弹(未爆)', markerfacecolor='lime', markeredgecolor='black', markersize=10),
        mpatches.Patch(color='gray', alpha=0.6, label='烟雾云团(已爆)'),
        Line2D([0], [0], marker='x', color='purple', label='假目标(方向)', markersize=10, linestyle='None')
    ]
    fig.legend(handles=legend_elements, loc='upper center', ncol=6, bbox_to_anchor=(0.5, 0.99))
    
    # --- 在图例下方添加三个独立的标题 ---
    fig.text(0.24, 0.88, '阶段1: 0-1.5s匀速直线飞行', ha='center', va='center', fontsize=12)
    fig.text(0.51, 0.88, '阶段2: 1.5s时刻烟雾弹投出未起爆', ha='center', va='center', fontsize=12)
    fig.text(0.78, 0.88, '阶段3: 5.1s后烟雾弹爆开', ha='center', va='center', fontsize=12)
    
    plt.tight_layout(rect=[0, 0.05, 1, 0.93])
    
    # --- 保存图像 ---
    output_filename = '2.问题一关键时刻空中态势图.png'
    plt.savefig(output_filename, dpi=300)
    print(f"图像已保存至: {output_filename}")


if __name__ == '__main__':
    main()