import numpy as np
import math
from config import *
from core_objects import UAV, Missile, TargetCylinder

def is_cloud_between_missile_and_target_internal(cloud_pos, missile_pos, target_pos, missile_unit_vec):
    """
    判断在起爆瞬间，烟云是否在空间上位于导弹和真目标之间。
    (修正版)
    """
    # 我们关心的是沿着导弹来袭的主要方向（可以近似为-x轴）的位置关系
    # 一个非常简单且有效的判断是直接比较 x 坐标，因为主要运动发生在x轴
    # 导弹从大x飞向小x，目标在x=0附近。
    # 条件： target_x < cloud_x < missile_x
    
    # 为了更通用，我们使用投影
    # 将所有点投影到导弹的飞行轴线上。
    # 我们可以把这条轴线想象成一把从无穷远处指向假目标的尺子。
    # 刻度值 = 点P在轴线上的投影 = P · missile_unit_vec
    
    # 导弹飞行方向单位向量
    u = missile_unit_vec
    
    proj_cloud = np.dot(cloud_pos, u)
    proj_missile = np.dot(missile_pos, u)
    proj_target = np.dot(target_pos, u)

    # u 是一个指向原点（假目标）的向量，所以其x,z分量为负。
    # 因此，x坐标越大的点，其投影值越小（越负）。
    # x_missile > x_cloud => proj_missile < proj_cloud
    # x_cloud > x_target => proj_cloud < proj_target (近似)

    # 所以，正确的顺序应该是 proj_missile < proj_cloud < proj_target
    return proj_missile < proj_cloud and proj_cloud < proj_target
def find_max_effective_deploy_time(uav_id, missile_id, fuse_time_options=[0.1, 20.0]):
    """
    通过分析极端投放策略，找到一个合理的 t_deploy 上边界。
    
    Args:
        uav_id (str): 无人机的ID, e.g., 'FY1'
        missile_id (str): 导弹的ID, e.g., 'M1'
        fuse_time_options (list): 用于测试的极端引信时间列表

    Returns:
        float: 计算出的 t_deploy 的最大有效时间。
    """
    uav = UAV(uav_id)
    missile = Missile(missile_id)
    target = TargetCylinder(TRUE_TARGET_SPECS)

    print(f"开始为 UAV({uav_id}) vs Missile({missile_id}) 计算 t_deploy 的有效上边界...")
    print("="*65)
    print("t_deploy(s) | 飞行角度(°) | 起爆点 X   | 导弹 X     | 是否满足约束?")
    print("-" * 65)

    # 扫描 t_deploy 的值
    for t_deploy in np.arange(0.1, 40.0, 0.5): # 以0.5秒为步长进行扫描
        
        # 对于给定的 t_deploy，我们需要找到能让烟云最靠前的策略
        # 即无人机飞行速度最大，飞行方向使投放点的x坐标最小。
        # 对于FY1和M1，它们y坐标初始几乎同线，最优方向就是沿着x轴负方向飞 (angle=pi)
        optimal_angle = np.pi 
        uav.set_flight_strategy(UAV_SPEED_MAX, optimal_angle)

        is_any_fuse_time_valid = False
        
        # 测试一组典型的或极端的引信时间
        for t_fuse in fuse_time_options:
            grenade = uav.deploy_grenade(t_deploy, t_fuse)
            t_b = grenade.detonate_time
            
            # 获取关键位置
            cloud_pos = grenade.detonate_pos
            missile_pos = missile.get_position(t_b)
            
            # 检查约束
            if is_cloud_between_missile_and_target_internal(
                cloud_pos=cloud_pos,
                missile_pos=missile_pos,
                target_pos=target.bottom_center,
                missile_unit_vec=missile.unit_vec
            ):
                is_any_fuse_time_valid = True
                
                # 打印一次有效信息即可
                print(
                    f"{t_deploy:<11.1f} | "
                    f"{math.degrees(optimal_angle):<12.1f} | "
                    f"{cloud_pos[0]:<10.1f} | "
                    f"{missile_pos[0]:<10.1f} | "
                    f"✔️ (t_fuse={t_fuse:.1f}s 有效)"
                )
                break # 只要有一个t_fuse有效，就认为这个t_deploy是有效的

        if not is_any_fuse_time_valid:
            # 如果所有测试的引信时间都无法满足约束，
            # 那么可以认为这个t_deploy已经太大了。
            # 我们取上一个有效的时间点作为边界。
            max_t_deploy = t_deploy - 0.5 # 回退一个步长
            print(
                f"{t_deploy:<11.1f} | "
                f"{math.degrees(optimal_angle):<12.1f} | "
                f"---        | ---        | ❌  <-- 临界点"
            )
            print("="*65)
            print(f"结论: 在 t_deploy ≈ {max_t_deploy:.1f} 秒之后，即使采用最极端策略也难以形成有效遮蔽。")
            return max_t_deploy
            
    # 如果扫描完都没找到无效点，说明上界可以设置得更大
    max_t_deploy_fallback = 40.0
    print("="*65)
    print(f"警告: 在扫描范围内未找到无效的 t_deploy，返回默认上界 {max_t_deploy_fallback:.1f}。")
    return max_t_deploy_fallback

if __name__ == '__main__':
    # 将此脚本作为独立工具运行，来为我们的优化问题确定边界
    
    # 确保你的 core_objects.py 和 config.py 是可用的
    # 并且 core_objects.py 中 Grenade 的弹道计算是你最终使用的版本（带风阻或不带风阻）
    
    # 针对问题3、4、5中的 FY1 vs M1 场景
    constrained_t_deploy_max = find_max_effective_deploy_time(uav_id='FY2', missile_id='M1')
    
    print(f"\n[最终建议] 将 t_deploy 的上边界设置为: {constrained_t_deploy_max:.2f}")