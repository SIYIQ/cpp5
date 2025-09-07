# 十个时间步长精度分析.py
from optimizer import ObscurationOptimizer
from config import *
import numpy as np
import matplotlib.pyplot as plt

class ExtendedDiscretizationAnalyzer:
    def __init__(self):
        # 使用问题二的最优解作为基准参数
        self.optimal_params = [79.6123, np.radians(7.28), 0.2306, 0.5782]
        # 10个不同的时间步长，从0.005到0.1
        self.time_steps = [0.1, 0.08, 0.06, 0.05, 0.04, 0.03, 0.02, 0.015, 0.01, 0.005]
        
    def calculate_precision_data(self):
        """计算10个时间步长的精度数据"""
        results = {}
        
        print("="*60)
        print("扩展离散化精度影响分析（10个时间步长）")
        print("="*60)
        
        for time_step in self.time_steps:
            print(f"正在计算时间步长: {time_step} s")
            
            # 创建优化器并设置时间步长
            optimizer = Problem2Optimizer(missile_id='M1', uav_assignments={'FY1': 1})
            optimizer.time_step = time_step
            
            # 使用最优参数计算遮蔽时间
            obscuration_time = -optimizer.objective_function(self.optimal_params)
            
            results[time_step] = obscuration_time
            print(f"  遮蔽时间: {obscuration_time:.4f} s")
        
        return results
    
    def plot_precision_line_chart(self, results):
        """绘制10个时间步长的精度影响折线图"""
        # 设置中文字体
        plt.rcParams['font.sans-serif'] = ['SimHei', 'Microsoft YaHei']
        plt.rcParams['axes.unicode_minus'] = False
        
        # 准备数据
        time_steps = sorted(results.keys(), reverse=True)  # 从大到小排序
        obscuration_times = [results[ts] for ts in time_steps]
        
        # 创建图形
        plt.figure(figsize=(12, 8))
        
        # 绘制折线图
        plt.plot(time_steps, obscuration_times, 
                color='darkgreen', 
                linewidth=3, 
                marker='o', 
                markersize=8, 
                markerfacecolor='orange',
                markeredgecolor='darkorange',
                markeredgewidth=2,
                label='遮蔽时间')
        
        # 添加数据标签
        for i, (x, y) in enumerate(zip(time_steps, obscuration_times)):
            plt.annotate(f'{y:.3f}s', 
                        (x, y), 
                        textcoords="offset points", 
                        xytext=(0, 15), 
                        ha='center',
                        fontsize=9,
                        fontweight='bold')
        
        # 设置坐标轴
        plt.xlabel('时间步长 (s)', fontsize=14, fontweight='bold')
        plt.ylabel('遮蔽时间 (s)', fontsize=14, fontweight='bold')
        plt.title('时间步长对计算精度的影响分析（10个数据点）', fontsize=16, fontweight='bold', pad=20)
        
        # 设置纵坐标刻度字体大小
        plt.yticks(fontsize=12)
        
        # 调整纵坐标范围以凸显变化
        y_min = min(obscuration_times) - 0.02
        y_max = max(obscuration_times) + 0.02
        plt.ylim(y_min, y_max)
        
        # 设置横坐标
        plt.xlim(0, 0.11)
        plt.xticks(time_steps, rotation=45, fontsize=12)
        
        # 添加网格
        plt.grid(True, alpha=0.3, linestyle='--')
        
        # 添加图例
        plt.legend(fontsize=12, loc='upper right')
        
        # 调整布局
        plt.tight_layout()
        
        # 保存图片
        plt.savefig('3.解答代码_solve1_2/十个时间步长精度影响折线图.png', dpi=300, bbox_inches='tight')
        plt.show()
        
        print("折线图已保存为: 十个时间步长精度影响折线图.png")
    
    def generate_detailed_table(self, results):
        """生成详细的精度分析表格"""
        print("\n" + "="*80)
        print("详细离散化精度分析数据表格（10个时间步长）")
        print("="*80)
        
        # 以最小时间步长的结果作为基准
        reference_time = results[min(self.time_steps)]
        
        print(f"{'时间步长 (s)':<12} {'遮蔽时间 (s)':<15} {'相对误差 (%)':<15} {'精度评价':<12} {'计算效率':<10}")
        print("-" * 80)
        
        # 计算模拟的计算时间（基于时间步长的倒数关系）
        base_time = 10.0  # 基准计算时间
        
        for time_step in sorted(self.time_steps, reverse=True):
            obscuration_time = results[time_step]
            relative_error = abs(obscuration_time - reference_time) / reference_time * 100
            calc_time = base_time * (0.01 / time_step)  # 反比关系
            
            # 精度评价
            if time_step >= 0.08:
                precision_level = "较粗糙"
            elif time_step >= 0.05:
                precision_level = "适中"
            elif time_step >= 0.02:
                precision_level = "较精细"
            elif time_step >= 0.01:
                precision_level = "高精度"
            else:
                precision_level = "超高精度"
            
            # 效率评价
            if calc_time <= 15:
                efficiency = "高效"
            elif calc_time <= 50:
                efficiency = "适中"
            elif calc_time <= 100:
                efficiency = "较慢"
            else:
                efficiency = "很慢"
            
            print(f"{time_step:<12} {obscuration_time:<15.4f} {relative_error:<15.2f} {precision_level:<12} {efficiency:<10}")
        
        # 统计分析
        print("\n" + "="*80)
        print("统计分析")
        print("="*80)
        
        times = list(results.values())
        print(f"遮蔽时间范围: [{min(times):.4f}, {max(times):.4f}] s")
        print(f"最大变化幅度: {max(times) - min(times):.4f} s")
        print(f"相对变化率: {(max(times) - min(times)) / min(times) * 100:.2f}%")
        print(f"标准差: {np.std(times):.4f} s")

class Problem2Optimizer(ObscurationOptimizer):
    def _parse_decision_variables(self, dv):
        # dv = [speed, angle, t_deploy, t_fuse]
        speed, angle, t_d, t_f = dv
        return {
            'FY1': {
                'speed': speed,
                'angle': angle,
                'grenades': [{'t_deploy': t_d, 't_fuse': t_f}]
            }
        }

if __name__ == '__main__':
    analyzer = ExtendedDiscretizationAnalyzer()
    
    print("开始扩展离散化精度影响分析...")
    results = analyzer.calculate_precision_data()
    
    # 生成结果表格
    analyzer.generate_detailed_table(results)
    
    # 绘制折线图
    print("\n正在生成折线图...")
    analyzer.plot_precision_line_chart(results)
    
    print("\n" + "="*60)
    print("扩展离散化精度分析完成！")
    print("="*60)
