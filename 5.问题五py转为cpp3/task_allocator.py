import numpy as np
from config import UAVS_INITIAL, MISSILES_INITIAL, UAV_SPEED_MAX

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

# --- 核心函数2: 任务分配 (保持不变) ---
def assign_tasks_by_threat(threat_weights):
    """
    根据威胁权重，使用贪心策略动态分配无人机。
    """
    uav_ids = list(UAVS_INITIAL.keys())
    missile_ids = list(MISSILES_INITIAL.keys())
    num_uavs = len(uav_ids)

    # 1. 根据权重决定每个导弹应分配的无人机数量
    print("\n--- 正在根据威胁权重计算资源需求 ---")
    allocation_requirements = {}
    temp_alloc = {m_id: int(np.round(threat_weights[m_id] * num_uavs)) for m_id in missile_ids}
    
    current_total = sum(temp_alloc.values())
    while current_total < num_uavs:
        m_id_to_add = max(threat_weights, key=threat_weights.get)
        temp_alloc[m_id_to_add] += 1
        current_total += 1
    while current_total > num_uavs:
        m_id_to_sub = min(threat_weights, key=threat_weights.get)
        if temp_alloc[m_id_to_sub] > 0:
            temp_alloc[m_id_to_sub] -= 1
        current_total -= 1
    
    allocation_requirements = temp_alloc
    for m_id, num in allocation_requirements.items():
        print(f"  导弹 {m_id} (权重 {threat_weights[m_id]:.2f}) -> 分配 {num} 架无人机")
    print("------------------------------------")

    # 2. 基于成本排序的贪心分配
    uav_missile_costs = {
        u_id: {m_id: calculate_engagement_time_cost(UAVS_INITIAL[u_id], MISSILES_INITIAL[m_id]) for m_id in missile_ids}
        for u_id in uav_ids
    }

    assignments = {m_id: {} for m_id in missile_ids}
    assigned_uavs = set()

    for missile_id in sorted(threat_weights, key=threat_weights.get, reverse=True):
        num_needed = allocation_requirements.get(missile_id, 0)
        if num_needed == 0: continue
        
        candidates = sorted(
            [(u_id, uav_missile_costs[u_id][missile_id]) for u_id in uav_ids if u_id not in assigned_uavs],
            key=lambda x: x[1]
        )
        
        for i in range(min(num_needed, len(candidates))):
            uav_to_assign = candidates[i][0]
            assignments[missile_id][uav_to_assign] = 3
            assigned_uavs.add(uav_to_assign)

    print("\n--- 最终任务分配方案 (基于威胁权重和成本) ---")
    for missile_id, uav_alloc in assignments.items():
        if not uav_alloc:
            print(f"  导弹 {missile_id} 未分配到拦截资源。")
            continue
        uav_list = list(uav_alloc.keys())
        print(f"  导弹 {missile_id} 由无人机 {', '.join(uav_list)} 进行拦截。")
    print("------------------------------------------------")
    
    return assignments

# --- 新增的主函数入口 ---
if __name__ == '__main__':
    """
    当这个脚本被直接运行时，执行此部分代码。
    这允许我们将此文件作为一个独立的任务分配模拟器来使用。
    """
    
    # 为了让这个脚本可以独立运行，我们需要从 threat_assessor.py 导入评估函数
    try:
        from threat_assessor import assess_threat_weights
    except ImportError:
        print("错误: 无法导入 'threat_assessor.py'。")
        print("请确保 threat_assessor.py 文件在同一目录下，并且包含了 assess_threat_weights 函数。")
        exit()

    print("="*60)
    print("      任务分配模块独立运行测试")
    print("="*60)
    
    # 步骤 1: 进行威胁评估，获取权重
    # 我们可以传入不同的专家权重来测试分配结果的变化
    expert_weights = {'tti': 0.5, 'crit': 0.3, 'diff': 0.2}
    print(f"\n使用的专家权重 (tti, crit, diff): {tuple(expert_weights.values())}")
    threat_weights = assess_threat_weights(factor_weights=expert_weights)
    
    # 步骤 2: 根据计算出的威胁权重，进行任务分配
    final_assignments = assign_tasks_by_threat(threat_weights)
    
    print("\n脚本独立运行测试完成。")
    print("="*60)