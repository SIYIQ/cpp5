# 模型离散化精度分析.py
from optimizer import ObscurationOptimizer
from config import *
import numpy as np
import matplotlib.pyplot as plt

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

class DiscretizationAnalyzer:
    def __init__(self):
        # 使用问题二的最优解作为基准参数
        self.optimal_params = [79.6123, np.radians(7.28), 0.2306, 0.5782]
        # 10个不同的时间步长，从0.005到0.1
        self.time_steps = [0.1, 0.08, 0.06, 0.05, 0.04, 0.03, 0.02, 0.015, 0.01, 0.005]
        
    def analyze_precision_impact(self):
        """分析不同时间步长对计算精度的影响"""
        results = {}
        convergence_data = {}
        
        print("="*60)
        print("离散化精度影响分析")
        print("="*60)
        
        for time_step in self.time_steps:
            print(f"\n正在分析时间步长: {time_step} s")
            
            # 创建优化器并设置时间步长
            optimizer = Problem2Optimizer(missile_id='M1', uav_assignments={'FY1': 1})
            optimizer.time_step = time_step
            
            # 使用最优参数计算遮蔽时间
            obscuration_time = -optimizer.objective_function(self.optimal_params)
            
            # 进行一次小规模优化来获取收敛曲线
            bounds = [
                (UAV_SPEED_MIN, UAV_SPEED_MAX),
                (0, 2 * np.pi),
                (0.1, 15),
                (0.1, 20.0)
            ]
            
            # 设置较小的种群和迭代次数以快速获取收敛数据
            solver_options = {
                'popsize': 60,  # 减小种群规模
                'maxiter': 300,  # 减少迭代次数
                'tol': 0.01,
                'disp': False,
                'workers': 1,
                'callback': self._create_callback(time_step)
            }
            
            # 重置收敛数据收集
            self.convergence_history = []
            
            try:
                optimal_strategy, max_time = optimizer.solve(bounds, **solver_options)
                convergence_data[time_step] = self.convergence_history.copy()
            except:
                print(f"优化失败，使用固定参数计算结果")
                convergence_data[time_step] = []
            
            results[time_step] = {
                'obscuration_time': obscuration_time,
                'optimization_time': max_time if 'max_time' in locals() else obscuration_time
            }
            
            print(f"  使用最优参数计算的遮蔽时间: {obscuration_time:.4f} s")
        
        return results, convergence_data
    
    def _create_callback(self, time_step):
        """创建回调函数来记录收敛过程"""
        def callback(xk, convergence=None):
            # 计算当前参数的目标函数值
            optimizer = Problem2Optimizer(missile_id='M1', uav_assignments={'FY1': 1})
            optimizer.time_step = time_step
            obj_value = -optimizer.objective_function(xk)  # 转换为正值
            self.convergence_history.append(obj_value)
        return callback
    
    def plot_convergence_curves(self, convergence_data):
        """绘制不同时间步长下的收敛曲线"""
        plt.figure(figsize=(14, 10))
        
        # 定义更多颜色和线型
        colors = ['blue', 'red', 'green', 'orange', 'purple', 'brown', 'pink', 'gray', 'olive', 'cyan']
        linestyles = ['-', '--', '-.', ':', '-', '--', '-.', ':', '-', '--']
        
        for i, (time_step, history) in enumerate(convergence_data.items()):
            if len(history) > 0:
                iterations = range(1, len(history) + 1)
                plt.plot(iterations, history, 
                        color=colors[i % len(colors)], 
                        linestyle=linestyles[i % len(linestyles)],
                        linewidth=2,
                        label=f'时间步长 = {time_step} s',
                        alpha=0.8)
        
        plt.xlabel('迭代轮数', fontsize=12)
        plt.ylabel('遮蔽时间 (s)', fontsize=12)
        plt.title('不同时间步长下的差分进化算法收敛曲线', fontsize=14, fontweight='bold')
        plt.legend(fontsize=9, ncol=2, loc='upper right')
        plt.grid(True, alpha=0.3)
        plt.tight_layout()
        
        # 保存图片
        plt.savefig('3.解答代码_solve1_2/离散化精度收敛曲线.png', dpi=300, bbox_inches='tight')
        plt.show()
        
        print("收敛曲线图已保存为: 离散化精度收敛曲线.png")
    
    def generate_precision_table(self, results):
        """生成精度分析表格数据"""
        print("\n" + "="*60)
        print("离散化精度分析结果表格")
        print("="*60)
        
        print(f"{'时间步长 (s)':<12} {'遮蔽时间 (s)':<12} {'相对误差 (%)':<12} {'精度评价':<10}")
        print("-" * 50)
        
        # 以最小时间步长的结果作为基准
        reference_time = results[min(self.time_steps)]['obscuration_time']
        
        for time_step in self.time_steps:
            obscuration_time = results[time_step]['obscuration_time']
            relative_error = abs(obscuration_time - reference_time) / reference_time * 100
            
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
                
            print(f"{time_step:<12} {obscuration_time:<12.4f} {relative_error:<12.2f} {precision_level:<10}")

if __name__ == '__main__':
    # 设置中文字体
    plt.rcParams['font.sans-serif'] = ['SimHei', 'Microsoft YaHei']
    plt.rcParams['axes.unicode_minus'] = False
    
    analyzer = DiscretizationAnalyzer()
    
    print("开始离散化精度影响分析...")
    results, convergence_data = analyzer.analyze_precision_impact()
    
    # 生成结果表格
    analyzer.generate_precision_table(results)
    
    # 绘制收敛曲线
    print("\n正在生成收敛曲线图...")
    analyzer.plot_convergence_curves(convergence_data)
    
    print("\n" + "="*60)
    print("离散化精度分析完成！")
    print("="*60)
