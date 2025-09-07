# config.py
import numpy as np

# --- 物理常量 ---
G = 9.8
CLOUD_SINK_SPEED = 3.0
CLOUD_RADIUS = 10.0
CLOUD_DURATION = 20.0
UAV_SPEED_MIN = 70.0
UAV_SPEED_MAX = 140.0
GRENADE_INTERVAL = 1.0
GRENADE_MASS = 5.0  # 烟幕弹质量 (kg)
GRENADE_DRAG_FACTOR = 0.005 # 阻力因子 k = 0.5 * C_d * ρ * A

# --- 目标信息 ---
TRUE_TARGET_SPECS = {
    'center_bottom': np.array([0.0, 200.0, 0.0]),
    'radius': 7.0,
    'height': 10.0
}

# --- 初始状态 (t=0) ---
MISSILES_INITIAL = {
    'M1': {'pos': np.array([20000.0, 0.0, 2000.0]), 'speed': 300.0, 'target': np.array([0.0, 0.0, 0.0])},
    'M2': {'pos': np.array([19000.0, 600.0, 2100.0]), 'speed': 300.0, 'target': np.array([0.0, 0.0, 0.0])},
    'M3': {'pos': np.array([18000.0, -600.0, 1900.0]), 'speed': 300.0, 'target': np.array([0.0, 0.0, 0.0])}
}

UAVS_INITIAL = {
    'FY1': {'pos': np.array([17800.0, 0.0, 1800.0])},
    'FY2': {'pos': np.array([12000.0, 1400.0, 1400.0])},
    'FY3': {'pos': np.array([6000.0, -3000.0, 700.0])},
    'FY4': {'pos': np.array([11000.0, 2000.0, 1800.0])},
    'FY5': {'pos': np.array([13000.0, -2000.0, 1300.0])}
}
