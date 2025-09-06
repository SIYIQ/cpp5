import numpy as np
from config import UAVS_INITIAL, MISSILES_INITIAL, UAV_SPEED_MAX
from scipy.optimize import milp

# --- 核心函数1: 成本计算 (保持不变) ---
def calculate_engagement_time_cost(uav_spec, missile_spec):
    """
    计算无人机拦截导弹的成本，以“最小反应时间”为标准。
    """
    uav_pos = uav_spec['pos']
    missile_start_pos = missile_spec['pos']
    missile_target_pos = missile_spec['target']
    intercept_point = missile_start_pos + (missile_target_pos - missile_start_pos) / 3.0
    distance_to_intercept = np.linalg.norm(uav_pos - intercept_point)
    time_to_intercept = distance_to_intercept / UAV_SPEED_MAX
    return time_to_intercept

def assign_tasks_by_optimal_allocation(threat_weights):
    """
    根据威胁权重确定各导弹所需无人机数量，
    然后使用整数线性规划 (ILP) 求解最优分配方案，以最小化总反应时间。
    """
    uav_ids = sorted(list(UAVS_INITIAL.keys()))
    missile_ids = sorted(list(MISSILES_INITIAL.keys()))
    num_uavs = len(uav_ids)
    num_missiles = len(missile_ids)

    # 1. 根据权重决定每个导弹应分配的无人机数量 (逻辑不变)
    print("\n--- 正在根据威胁权重计算资源需求 ---")
    allocation_requirements = {}
    temp_alloc = {m_id: int(np.round(threat_weights[m_id] * num_uavs)) for m_id in missile_ids}
    
    current_total = sum(temp_alloc.values())
    # 确保分配的无人机总数等于现有的无人机总数
    while current_total < num_uavs:
        # 如果分配总数小于无人机数，将多余的无人机分配给威胁最高的导弹
        m_id_to_add = max(threat_weights, key=threat_weights.get)
        temp_alloc[m_id_to_add] += 1
        current_total += 1
    while current_total > num_uavs:
        # 如果分配总数大于无人机数，从威胁最低的导弹处减少分配
        m_id_to_sub = min(threat_weights, key=threat_weights.get)
        if temp_alloc[m_id_to_sub] > 0:
            temp_alloc[m_id_to_sub] -= 1
            current_total -= 1
    
    allocation_requirements = temp_alloc
    for m_id, num in allocation_requirements.items():
        print(f"  导弹 {m_id} (权重 {threat_weights[m_id]:.2f}) -> 分配 {num} 架无人机")
    print("------------------------------------")

    # 2. 构建成本矩阵
    cost_matrix = np.zeros((num_uavs, num_missiles))
    for i, u_id in enumerate(uav_ids):
        for j, m_id in enumerate(missile_ids):
            cost_matrix[i, j] = calculate_engagement_time_cost(UAVS_INITIAL[u_id], MISSILES_INITIAL[m_id])

    # 3. 设置整数线性规划 (ILP)
    # 决策变量 x_ij, 如果无人机 i 分配给导弹 j, 则为 1, 否则为 0
    # 目标函数: 最小化 sum(cost_ij * x_ij)
    c = cost_matrix.flatten()

    # 约束1: 每个无人机最多分配一次
    # sum(x_ij for j in missiles) <= 1  (for each uav i)
    A_ub = np.zeros((num_uavs, num_uavs * num_missiles))
    for i in range(num_uavs):
        for j in range(num_missiles):
            A_ub[i, i * num_missiles + j] = 1
    b_ub = np.ones(num_uavs)

    # 约束2: 每个导弹必须分配到指定数量的无人机
    # sum(x_ij for i in uavs) == N_j (for each missile j)
    A_eq = np.zeros((num_missiles, num_uavs * num_missiles))
    for j in range(num_missiles):
        for i in range(num_uavs):
            A_eq[j, i * num_missiles + j] = 1
    b_eq = np.array([allocation_requirements[m_id] for m_id in missile_ids])

    # 求解
    print("\n--- 正在使用 ILP 求解最优分配方案 ---")
    # 变量都是二进制的 (0 或 1)
    integrality = np.ones_like(c)
    
    res = milp(c=c, constr=[(A_ub, b_ub, b_ub), (A_eq, b_eq, b_eq)], integrality=integrality)

    # 4. 解析结果
    assignments = {m_id: {} for m_id in missile_ids}
    if res.success:
        print("  ILP 求解成功，找到最优解!")
        x = np.round(res.x).reshape((num_uavs, num_missiles))
        for i in range(num_uavs):
            for j in range(num_missiles):
                if x[i, j] == 1:
                    uav_id = uav_ids[i]
                    missile_id = missile_ids[j]
                    assignments[missile_id][uav_id] = 3 # 默认分配3枚弹药
    else:
        print("  ILP 求解失败! 请检查模型。")
        # 在这里可以回退到贪心算法或抛出错误
        return {}

    print("\n--- 最终任务分配方案 (最优解) ---")
    for missile_id, uav_alloc in assignments.items():
        if not uav_alloc:
            print(f"  导弹 {missile_id} 未分配到拦截资源。")
            continue
        uav_list = list(uav_alloc.keys())
        print(f"  导弹 {missile_id} 由无人机 {', '.join(uav_list)} 进行拦截。")
    print("------------------------------------------------")
    
    return assignments

# 为了兼容旧的函数名，我们将原来的函数重命名并让旧名字指向新函数
assign_tasks_by_threat = assign_tasks_by_optimal_allocation

# --- 主函数入口 (保持不变) ---
if __name__ == '__main__':
    """
    当这个脚本被直接运行时，执行此部分代码。
    """
    try:
        from threat_assessor import assess_threat_weights
    except ImportError:
        print("错误: 无法导入 'threat_assessor.py'。")
        exit()

    print("="*60)
    print("      任务分配模块独立运行测试 (使用 ILP 最优分配)")
    print("="*60)
    
    expert_weights = {'tti': 0.5, 'crit': 0.3, 'diff': 0.2}
    print(f"\n使用的专家权重 (tti, crit, diff): {tuple(expert_weights.values())}")
    threat_weights = assess_threat_weights(factor_weights=expert_weights)
    
    final_assignments = assign_tasks_by_threat(threat_weights)
    
    print("\n脚本独立运行测试完成。")
    print("="*60)
