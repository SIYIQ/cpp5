# threat_assessor.py
import numpy as np
from config import MISSILES_INITIAL, TRUE_TARGET_SPECS

class FactorWeights:
    """威胁因子权重结构"""
    def __init__(self, tti=0.5, crit=0.3, diff=0.2):
        self.tti = tti      # time-to-impact 权重
        self.crit = crit    # criticality 权重  
        self.diff = diff    # difficulty 权重

class ThreatMetrics:
    """威胁评估结果"""
    def __init__(self, time_to_impact, criticality, difficulty, overall_threat):
        self.time_to_impact = time_to_impact
        self.criticality = criticality
        self.difficulty = difficulty
        self.overall_threat = overall_threat

def calculate_time_to_impact(missile_id):
    """计算导弹到达目标的时间"""
    if missile_id not in MISSILES_INITIAL:
        return 1000.0  # 默认很大的时间
    
    missile_spec = MISSILES_INITIAL[missile_id]
    direction = missile_spec['target'] - missile_spec['pos']
    distance = np.linalg.norm(direction)
    return distance / missile_spec['speed']

def calculate_criticality(missile_id):
    """计算关键性评分（基于初始位置和速度）"""
    if missile_id not in MISSILES_INITIAL:
        return 0.5  # 默认中等关键性
    
    missile_spec = MISSILES_INITIAL[missile_id]
    
    # 基于初始位置的关键性：距离目标越近，关键性越高
    distance_to_target = np.linalg.norm(missile_spec['pos'] - missile_spec['target'])
    
    # 基于速度的关键性：速度越快，关键性越高
    speed_factor = missile_spec['speed'] / 400.0  # 归一化到400m/s
    
    # 基于高度的关键性：高度适中的导弹更难拦截
    altitude = missile_spec['pos'][2]
    altitude_factor = 1.0 - abs(altitude - 2000.0) / 2000.0  # 2000m为最优高度
    
    # 综合评分
    distance_score = max(0.0, 1.0 - distance_to_target / 25000.0)
    speed_score = min(1.0, speed_factor)
    altitude_score = max(0.0, altitude_factor)
    
    return (distance_score * 0.5 + speed_score * 0.3 + altitude_score * 0.2)

def calculate_difficulty(missile_id):
    """计算拦截难度评分"""
    if missile_id not in MISSILES_INITIAL:
        return 0.5  # 默认中等难度
    
    missile_spec = MISSILES_INITIAL[missile_id]
    
    # 基于初始位置偏离程度的难度
    target_center = TRUE_TARGET_SPECS['center_bottom']
    missile_pos = missile_spec['pos']
    
    # Y方向偏离（侧向偏离）
    lateral_deviation = abs(missile_pos[1] - target_center[1])
    
    # Z方向偏离（高度偏离）  
    altitude_deviation = abs(missile_pos[2] - 2000.0)  # 假设2000m为标准高度
    
    # 距离因子
    distance = np.linalg.norm(missile_pos - target_center)
    
    # 综合难度评分
    lateral_score = min(1.0, lateral_deviation / 1000.0)  # 归一化
    altitude_score = min(1.0, altitude_deviation / 1000.0)
    distance_score = min(1.0, distance / 20000.0)
    
    return (lateral_score * 0.4 + altitude_score * 0.3 + distance_score * 0.3)

def assess_single_missile_threat(missile_id, factor_weights=None):
    """评估单个导弹的威胁权重"""
    if factor_weights is None:
        factor_weights = FactorWeights()
    
    tti = calculate_time_to_impact(missile_id)
    crit = calculate_criticality(missile_id)
    diff = calculate_difficulty(missile_id)
    
    # 时间到达威胁：时间越短威胁越高
    tti_score = 1.0 / (1.0 + tti / 60.0)  # 归一化，60秒为参考时间
    
    # 综合威胁评分
    overall_threat = (factor_weights.tti * tti_score + 
                     factor_weights.crit * crit + 
                     factor_weights.diff * diff)
    
    return ThreatMetrics(tti, crit, diff, overall_threat)

def assess_threat_weights(factor_weights=None):
    """评估所有导弹的威胁权重"""
    if factor_weights is None:
        factor_weights = FactorWeights()
    
    threat_weights = {}
    threat_scores = []
    
    # 计算每个导弹的威胁评分
    for missile_id in MISSILES_INITIAL.keys():
        metrics = assess_single_missile_threat(missile_id, factor_weights)
        threat_scores.append((missile_id, metrics.overall_threat))
    
    # 归一化威胁权重，使总和为1.0
    total_threat = sum(score for _, score in threat_scores)
    
    if total_threat > 0.0:
        for missile_id, score in threat_scores:
            threat_weights[missile_id] = score / total_threat
    else:
        # 如果所有威胁评分都为0，平均分配权重
        equal_weight = 1.0 / len(MISSILES_INITIAL)
        for missile_id in MISSILES_INITIAL.keys():
            threat_weights[missile_id] = equal_weight
    
    # 打印威胁评估结果
    print("\n--- 威胁评估结果 ---")
    for missile_id, weight in threat_weights.items():
        metrics = assess_single_missile_threat(missile_id, factor_weights)
        print(f"导弹 {missile_id}: 威胁权重={weight:.3f} "
              f"(TTI={metrics.time_to_impact:.1f}s, "
              f"关键性={metrics.criticality:.3f}, "
              f"难度={metrics.difficulty:.3f})")
    print("-------------------")
    
    return threat_weights

if __name__ == '__main__':
    """测试威胁评估功能"""
    print("="*50)
    print("      威胁评估模块独立运行测试")
    print("="*50)
    
    # 使用默认权重进行评估
    threat_weights = assess_threat_weights()
    
    print(f"\n威胁权重总和: {sum(threat_weights.values()):.6f}")
    print("测试完成。")
    print("="*50)
