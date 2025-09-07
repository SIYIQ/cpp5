# 离散化精度折线图.py
import matplotlib.pyplot as plt
import numpy as np

def plot_discretization_precision():
    """绘制离散化精度影响的折线图"""
    
    # 已知数据
    time_steps = [0.1, 0.05, 0.02, 0.01]  # 时间步长 (s)
    obscuration_times = [4.7000, 4.6500, 4.6200, 4.6300]  # 遮蔽时间 (s)
    
    # 设置中文字体
    plt.rcParams['font.sans-serif'] = ['SimHei', 'Microsoft YaHei']
    plt.rcParams['axes.unicode_minus'] = False
    
    # 创建图形
    plt.figure(figsize=(10, 6))
    
    # 绘制折线图
    plt.plot(time_steps, obscuration_times, 
             color='blue', 
             linewidth=3, 
             marker='o', 
             markersize=8, 
             markerfacecolor='red',
             markeredgecolor='darkred',
             markeredgewidth=2,
             label='遮蔽时间')
    
    # 添加数据标签
    for i, (x, y) in enumerate(zip(time_steps, obscuration_times)):
        plt.annotate(f'{y:.4f} s', 
                    (x, y), 
                    textcoords="offset points", 
                    xytext=(0,15), 
                    ha='center',
                    fontsize=11,
                    fontweight='bold')
    
    # 设置坐标轴
    plt.xlabel('时间步长 (s)', fontsize=14, fontweight='bold')
    plt.ylabel('遮蔽时间 (s)', fontsize=14, fontweight='bold')
    plt.title('时间步长对计算精度的影响分析', fontsize=16, fontweight='bold', pad=20)
    
    # 调整纵坐标范围以凸显变化
    y_min = min(obscuration_times) - 0.05
    y_max = max(obscuration_times) + 0.05
    plt.ylim(y_min, y_max)
    
    # 设置横坐标
    plt.xlim(0, 0.11)
    plt.xticks(time_steps)
    
    # 添加网格
    plt.grid(True, alpha=0.3, linestyle='--')
    
    # 添加图例
    plt.legend(fontsize=12, loc='upper right')
    
    # 调整布局
    plt.tight_layout()
    
    # 保存图片
    plt.savefig('3.解答代码_solve1_2/离散化精度影响折线图.png', dpi=300, bbox_inches='tight')
    plt.show()
    
    print("折线图已保存为: 离散化精度影响折线图.png")
    
    # 输出数据表格
    print("\n" + "="*60)
    print("离散化精度分析数据表格")
    print("="*60)
    
    # 计算相对误差（以最小时间步长0.01s为基准）
    reference_time = obscuration_times[-1]  # 0.01s对应的遮蔽时间
    
    print(f"{'时间步长 (s)':<15} {'遮蔽时间 (s)':<15} {'相对误差 (%)':<15} {'精度评价':<10}")
    print("-" * 65)
    
    precision_levels = ["较粗糙", "适中", "较精细", "高精度"]
    
    for i, (time_step, obscuration_time) in enumerate(zip(time_steps, obscuration_times)):
        relative_error = abs(obscuration_time - reference_time) / reference_time * 100
        precision_level = precision_levels[i]
        
        print(f"{time_step:<15} {obscuration_time:<15.4f} {relative_error:<15.2f} {precision_level:<10}")
    
    # 计算计算时间（模拟数据）
    print("\n" + "="*60)
    print("计算效率对比")
    print("="*60)
    
    # 模拟的计算时间数据（基于时间步长的倒数关系）
    base_time = 12.3  # 0.1s的基准计算时间
    calc_times = [base_time, base_time*2, base_time*5, base_time*10]
    
    print(f"{'时间步长 (s)':<15} {'遮蔽时间 (s)':<15} {'计算时间 (s)':<15} {'效率评价':<10}")
    print("-" * 65)
    
    efficiency_levels = ["高效", "适中", "较慢", "很慢"]
    
    for i, (time_step, obscuration_time, calc_time) in enumerate(zip(time_steps, obscuration_times, calc_times)):
        efficiency_level = efficiency_levels[i]
        print(f"{time_step:<15} {obscuration_time:<15.4f} {calc_time:<15.1f} {efficiency_level:<10}")

if __name__ == '__main__':
    print("="*60)
    print("离散化精度影响分析 - 折线图生成")
    print("="*60)
    
    plot_discretization_precision()
    
    print("\n" + "="*60)
    print("分析完成！")
    print("="*60)
