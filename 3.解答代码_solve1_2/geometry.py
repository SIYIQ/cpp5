# geometry.py
import numpy as np
from config import CLOUD_RADIUS

def check_circle_in_cone(cone_vertex, cone_axis_unit_vec, cone_half_angle, circle_center, circle_radius, circle_normal):
    """
    基于解析法，判断一个圆周是否完全在圆锥内部。
    这是 "核心三" 推导的直接代码实现。
    """
    # 找到 V, C, d 定义的平面与圆周的两个交点
    C = circle_center
    V = cone_vertex
    d = cone_axis_unit_vec
    r = circle_radius
    n_c = circle_normal

    vec_vc = C - V
    vec_vc_norm = np.linalg.norm(vec_vc)
    
    # 特殊情况：V, C, d 共线，说明圆心在锥轴上，所有圆周点等价
    if np.linalg.norm(np.cross(vec_vc, d)) < 1e-6 * vec_vc_norm:
        # 随便取一个圆周点即可
        # 构造一个与 n_c 垂直的向量
        if abs(n_c[0]) < 0.9:
            perp_vec = np.array([1.0, 0.0, 0.0])
        else:
            perp_vec = np.array([0.0, 1.0, 0.0])
        ortho_vec = np.cross(n_c, perp_vec)
        extreme_point = C + r * (ortho_vec / np.linalg.norm(ortho_vec))
        points_to_check = [extreme_point]
    else:
        # 一般情况：找到两个极值点
        plane_normal = np.cross(vec_vc, d)
        intersection_dir = np.cross(plane_normal, n_c)
        unit_intersection_dir = intersection_dir / np.linalg.norm(intersection_dir)
        
        p1 = C + r * unit_intersection_dir
        p2 = C - r * unit_intersection_dir
        points_to_check = [p1, p2]

    # 检查这些极值点是否满足在锥内的条件
    for p in points_to_check:
        vec_vp = p - V
        vec_vp_norm = np.linalg.norm(vec_vp)
        
        if vec_vp_norm < 1e-9:  # 点P与顶点V重合
            continue

        cos_beta = np.dot(vec_vp, d) / vec_vp_norm
        beta = np.arccos(np.clip(cos_beta, -1.0, 1.0))
        
        if beta > cone_half_angle:
            return False
            
    return True

def check_full_obscuration(missile_pos, cloud_center, target_cylinder):
    """
    判断在给定时刻，单个烟幕云团是否完全遮蔽了目标圆柱体。
    这是 "核心二" 凸包理论的应用。
    """
    # 1. 构建阴影锥
    cone_vertex = missile_pos
    dist_to_cloud = np.linalg.norm(cloud_center - cone_vertex)
    
    # 如果导弹已进入烟幕云团，视为完全遮蔽
    if dist_to_cloud <= CLOUD_RADIUS:
        return True
    
    cone_axis_unit_vec = (cloud_center - cone_vertex) / dist_to_cloud
    cone_half_angle = np.arcsin(CLOUD_RADIUS / dist_to_cloud)

    # 2. 检查上下两个底面圆盘是否都在圆锥内
    # 根据核心二的凸包理论，这等价于检查整个圆柱体
    
    # 检查底面圆
    bottom_circle_ok = check_circle_in_cone(
        cone_vertex, cone_axis_unit_vec, cone_half_angle,
        target_cylinder.bottom_center, target_cylinder.radius, np.array([0., 0., -1.])
    )
    if not bottom_circle_ok:
        return False
        
    # 检查顶面圆
    top_circle_ok = check_circle_in_cone(
        cone_vertex, cone_axis_unit_vec, cone_half_angle,
        target_cylinder.top_center, target_cylinder.radius, np.array([0., 0., 1.])
    )
    if not top_circle_ok:
        return False

    return True
