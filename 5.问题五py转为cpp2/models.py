# # models.py
# import numpy as np
# from config import *

# # --- 运动模型 ---
# def get_missile_info(missile_id):
#     # 预计算导弹的单位方向向量
#     start_pos = MISSILES_INITIAL[missile_id]['pos']
#     target_pos = MISSILES_INITIAL[missile_id]['target']
#     direction_vec = target_pos - start_pos
#     unit_vec = direction_vec / np.linalg.norm(direction_vec)
#     return {'start_pos': start_pos, 'speed': MISSILES_INITIAL[missile_id]['speed'], 'unit_vec': unit_vec}

# def get_missile_position(missile_info, t):
#     return missile_info['start_pos'] + missile_info['unit_vec'] * missile_info['speed'] * t

# def get_uav_position(uav_start_pos, t, uav_speed, uav_direction_angle):
#     uav_unit_vec = np.array([np.cos(uav_direction_angle), np.sin(uav_direction_angle), 0])
#     return uav_start_pos + uav_unit_vec * uav_speed * t

# def get_grenade_trajectory(deploy_pos, deploy_vel, t_since_deploy):
#     # t_since_deploy 是从投放时刻开始计算的时间
#     dx = deploy_vel[0] * t_since_deploy
#     dy = deploy_vel[1] * t_since_deploy
#     dz = deploy_vel[2] * t_since_deploy - 0.5 * G * t_since_deploy**2
#     return deploy_pos + np.array([dx, dy, dz])

# def get_cloud_center(detonate_pos, t_since_detonate):
#     # t_since_detonate 是从起爆时刻开始计算的时间
#     return detonate_pos - np.array([0, 0, CLOUD_SINK_SPEED * t_since_detonate])

# # 可以在 models.py 中新增或替换原有函数
# def check_full_obscuration_analytical(missile_pos, cloud_center, target_cylinder):
#     """
#     使用解析法判断圆柱体是否被完全遮蔽。
#     target_cylinder: 一个包含圆柱信息的对象或字典。
#     """
#     # 1. 构建阴影锥
#     cone_vertex = missile_pos
#     dist_to_cloud = np.linalg.norm(cloud_center - cone_vertex)
#     if dist_to_cloud <= CLOUD_RADIUS: return True
    
#     cone_axis_unit_vec = (cloud_center - cone_vertex) / dist_to_cloud
#     cone_half_angle = np.arcsin(CLOUD_RADIUS / dist_to_cloud)

#     # 2. 对上下两个底面圆周进行判断
#     circles_to_check = [
#         {'center': target_cylinder.bottom_center, 'normal': np.array([0, 0, -1])},
#         {'center': target_cylinder.top_center, 'normal': np.array([0, 0, 1])}
#     ]
    
#     max_beta_overall = 0.0

#     for circle in circles_to_check:
#         C = circle['center']
#         n_circle = circle['normal']
        
#         # 3. 找到关键平面与圆周的交点
#         # V, C, d 构成的平面的法向量 (d 是 cone_axis_unit_vec)
#         # 为避免 V-C 与 d 共线导致叉乘为0，做一个检查
#         vec_vc = C - cone_vertex
#         if np.linalg.norm(np.cross(vec_vc, cone_axis_unit_vec)) < 1e-6:
#              # V, C, d 共线，说明圆心在锥轴上，所有圆周点等价
#              # 随便取一个圆周点即可
#              # 构造一个与n_circle垂直的向量
#              perp_vec = np.array([1,0,0]) if np.abs(n_circle[0])<0.9 else np.array([0,1,0])
#              ortho_vec = np.cross(n_circle, perp_vec)
#              P1 = C + target_cylinder.radius * (ortho_vec / np.linalg.norm(ortho_vec))
#              P2 = P1 # 两个点等价
#         else:
#             n_plane = np.cross(vec_vc, cone_axis_unit_vec)
#             # 两个平面的交线方向
#             dir_intersection = np.cross(n_plane, n_circle)
#             unit_dir_intersection = dir_intersection / np.linalg.norm(dir_intersection)
            
#             # 计算两个交点
#             P1 = C + target_cylinder.radius * unit_dir_intersection
#             P2 = C - target_cylinder.radius * unit_dir_intersection

#         # 4. 检查这两个交点的夹角
#         for P in [P1, P2]:
#             vec_to_point = P - cone_vertex
#             vec_to_point_norm = np.linalg.norm(vec_to_point)
#             if vec_to_point_norm < 1e-6: continue

#             cos_beta = np.dot(vec_to_point, cone_axis_unit_vec) / vec_to_point_norm
#             beta = np.arccos(np.clip(cos_beta, -1.0, 1.0))
#             if beta > max_beta_overall:
#                 max_beta_overall = beta
                
#     # 5. 最终判断
#     return max_beta_overall <= cone_half_angle

# # --- 几何判断模型 (核心) ---
# # def check_full_obscuration(missile_pos, cloud_center):
# #     """
# #     判断在给定时刻，单个烟幕云团是否完全遮蔽了目标。
# #     返回: True or False
# #     """
# #     # 1. 构建阴影锥
# #     cone_vertex = missile_pos
# #     dist_to_cloud = np.linalg.norm(cloud_center - cone_vertex)
# #     if dist_to_cloud <= CLOUD_RADIUS: # 导弹已进入烟幕
# #         return True
    
# #     cone_axis_unit_vec = (cloud_center - cone_vertex) / dist_to_cloud
# #     cone_half_angle = np.arcsin(CLOUD_RADIUS / dist_to_cloud)

# #     # 2. 检查所有目标关键点是否在锥内
# #     max_beta = 0.0
# #     for point in TARGET_KEY_POINTS:
# #         vec_to_point = point - cone_vertex
# #         vec_to_point_norm = np.linalg.norm(vec_to_point)
# #         if vec_to_point_norm < 1e-6: continue

# #         cos_beta = np.dot(vec_to_point, cone_axis_unit_vec) / vec_to_point_norm
# #         beta = np.arccos(np.clip(cos_beta, -1.0, 1.0)) # clip避免浮点误差
# #         if beta > max_beta:
# #             max_beta = beta
            
# #     # 3. 判断
# #     return max_beta <= cone_half_angle
