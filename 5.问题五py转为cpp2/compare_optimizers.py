# compare_optimizers.py - 优化器性能对比脚本
import numpy as np
import time
import matplotlib.pyplot as plt
from optimizer import ObscurationOptimizer
from solve_problem_5 import Problem5SubOptimizer
from config import *
from boundary_calculator import find_max_effective_deploy_time


def create_test_case():
    """创建一个测试用例"""
    # 简单的测试场景：1个导弹，2个无人机
    missile_id = 'M1'
    uav_assignments = {'FY1': 2, 'FY2': 2}  # 每个UAV 2发弹药
    
    # 构建边界
    bounds = []
    uav_ids_for_task = sorted(uav_assignments.keys())
    
    for uav_id in uav_ids_for_task:
        num_grenades = uav_assignments[uav_id]
        t_max = find_max_effective_deploy_time(uav_id, missile_id)
        
        # 飞行参数边界
        bounds.extend([(UAV_SPEED_MIN, UAV_SPEED_MAX), (0, 2 * np.pi)])
        # 第一枚弹药边界
        bounds.extend([(0.1, t_max), (0.1, 20.0)])
        # 剩余弹药边界
        for _ in range(num_grenades - 1):
            bounds.extend([(GRENADE_INTERVAL, 10.0), (0.1, 20.0)])
    
    return missile_id, uav_assignments, bounds


def benchmark_optimizer(missile_id, uav_assignments, bounds, use_adaptive_de, num_runs=3):
    """对单个优化器进行基准测试"""
    results = {
        'times': [],
        'best_fitness': [],
        'iterations': [],
        'convergence_histories': []
    }
    
    print(f"\n{'='*50}")
    print(f"测试 {'自适应DE' if use_adaptive_de else '标准DE'} 算法")
    print(f"{'='*50}")
    
    for run in range(num_runs):
        print(f"\n--- 运行 {run + 1}/{num_runs} ---")
        
        # 创建优化器
        optimizer = Problem5SubOptimizer(
            missile_id=missile_id, 
            uav_assignments=uav_assignments, 
            use_adaptive_de=use_adaptive_de
        )
        
        # 设置参数
        D = len(bounds)
        if use_adaptive_de:
            solver_options = {
                'popsize': min(10 * D, 120),  # 较小的种群用于快速测试
                'maxiter': 300,
                'tol': 0.01,
                'disp': False,  # 减少输出
                'seed': 42 + run  # 固定种子确保可重现
            }
        else:
            solver_options = {
                'popsize': 10 * D,
                'maxiter': 300,
                'tol': 0.01,
                'disp': False,
                'workers': 1,  # 单线程确保公平对比
                'seed': 42 + run
            }
        
        # 执行优化
        start_time = time.time()
        try:
            optimal_strategy, max_time = optimizer.solve(bounds, **solver_options)
            end_time = time.time()
            
            execution_time = end_time - start_time
            best_fitness = -max_time  # 转换回最小化问题的适应度
            
            results['times'].append(execution_time)
            results['best_fitness'].append(best_fitness)
            
            print(f"  运行时间: {execution_time:.2f}s")
            print(f"  最佳遮蔽时间: {max_time:.4f}s")
            print(f"  最佳适应度: {best_fitness:.6f}")
            
        except Exception as e:
            print(f"  运行失败: {e}")
            continue
    
    # 计算统计信息
    if results['times']:
        avg_time = np.mean(results['times'])
        avg_fitness = np.mean(results['best_fitness'])
        std_fitness = np.std(results['best_fitness'])
        
        print(f"\n统计结果:")
        print(f"  平均运行时间: {avg_time:.2f} ± {np.std(results['times']):.2f}s")
        print(f"  平均适应度: {avg_fitness:.6f} ± {std_fitness:.6f}")
        print(f"  最佳适应度: {min(results['best_fitness']):.6f}")
        
        results['avg_time'] = avg_time
        results['avg_fitness'] = avg_fitness
        results['std_fitness'] = std_fitness
        results['best_fitness_overall'] = min(results['best_fitness'])
    
    return results


def compare_performance():
    """性能对比主函数"""
    print("🚀 差分进化算法性能对比")
    print("=" * 60)
    
    # 创建测试用例
    missile_id, uav_assignments, bounds = create_test_case()
    D = len(bounds)
    
    print(f"测试配置:")
    print(f"  导弹: {missile_id}")
    print(f"  UAV分配: {uav_assignments}")
    print(f"  优化维度: {D}")
    print(f"  边界数量: {len(bounds)}")
    
    # 测试两种算法
    num_runs = 5  # 运行次数
    
    # 测试标准DE
    standard_results = benchmark_optimizer(
        missile_id, uav_assignments, bounds, 
        use_adaptive_de=False, num_runs=num_runs
    )
    
    # 测试自适应DE
    adaptive_results = benchmark_optimizer(
        missile_id, uav_assignments, bounds, 
        use_adaptive_de=True, num_runs=num_runs
    )
    
    # 性能对比分析
    print(f"\n{'='*60}")
    print("📊 性能对比分析")
    print(f"{'='*60}")
    
    if (standard_results['times'] and adaptive_results['times'] and 
        standard_results['best_fitness'] and adaptive_results['best_fitness']):
        
        # 时间对比
        time_improvement = (standard_results['avg_time'] - adaptive_results['avg_time']) / standard_results['avg_time'] * 100
        
        # 解质量对比
        quality_improvement = (standard_results['avg_fitness'] - adaptive_results['avg_fitness']) / abs(standard_results['avg_fitness']) * 100
        
        print(f"⏱️  运行时间对比:")
        print(f"   标准DE:    {standard_results['avg_time']:.2f}s")
        print(f"   自适应DE:  {adaptive_results['avg_time']:.2f}s")
        print(f"   改进:      {time_improvement:+.1f}%")
        
        print(f"\n🎯 解质量对比:")
        print(f"   标准DE:    {standard_results['avg_fitness']:.6f} ± {standard_results['std_fitness']:.6f}")
        print(f"   自适应DE:  {adaptive_results['avg_fitness']:.6f} ± {adaptive_results['std_fitness']:.6f}")
        print(f"   改进:      {quality_improvement:+.2f}%")
        
        print(f"\n🏆 最佳解对比:")
        print(f"   标准DE:    {standard_results['best_fitness_overall']:.6f}")
        print(f"   自适应DE:  {adaptive_results['best_fitness_overall']:.6f}")
        
        # 稳定性对比
        standard_cv = standard_results['std_fitness'] / abs(standard_results['avg_fitness']) * 100
        adaptive_cv = adaptive_results['std_fitness'] / abs(adaptive_results['avg_fitness']) * 100
        
        print(f"\n📈 稳定性对比 (变异系数):")
        print(f"   标准DE:    {standard_cv:.2f}%")
        print(f"   自适应DE:  {adaptive_cv:.2f}%")
        print(f"   稳定性:    {'提升' if adaptive_cv < standard_cv else '下降'}")
        
        # 生成可视化对比图
        try:
            create_comparison_plot(standard_results, adaptive_results)
        except Exception as e:
            print(f"⚠️  图表生成失败: {e}")
    
    print(f"\n{'='*60}")
    print("✅ 性能对比完成")
    print(f"{'='*60}")


def create_comparison_plot(standard_results, adaptive_results):
    """创建对比图表"""
    fig, axes = plt.subplots(2, 2, figsize=(12, 10))
    fig.suptitle('差分进化算法性能对比', fontsize=16, fontweight='bold')
    
    # 运行时间对比
    axes[0, 0].boxplot([standard_results['times'], adaptive_results['times']], 
                       labels=['标准DE', '自适应DE'])
    axes[0, 0].set_title('运行时间对比')
    axes[0, 0].set_ylabel('时间 (秒)')
    axes[0, 0].grid(True, alpha=0.3)
    
    # 适应度对比
    axes[0, 1].boxplot([standard_results['best_fitness'], adaptive_results['best_fitness']], 
                       labels=['标准DE', '自适应DE'])
    axes[0, 1].set_title('解质量对比')
    axes[0, 1].set_ylabel('适应度')
    axes[0, 1].grid(True, alpha=0.3)
    
    # 适应度分布
    axes[1, 0].hist(standard_results['best_fitness'], alpha=0.7, label='标准DE', bins=10)
    axes[1, 0].hist(adaptive_results['best_fitness'], alpha=0.7, label='自适应DE', bins=10)
    axes[1, 0].set_title('适应度分布')
    axes[1, 0].set_xlabel('适应度')
    axes[1, 0].set_ylabel('频次')
    axes[1, 0].legend()
    axes[1, 0].grid(True, alpha=0.3)
    
    # 性能改进柱状图
    metrics = ['运行时间', '平均适应度', '最佳适应度']
    standard_vals = [np.mean(standard_results['times']), 
                    np.mean(standard_results['best_fitness']),
                    min(standard_results['best_fitness'])]
    adaptive_vals = [np.mean(adaptive_results['times']), 
                    np.mean(adaptive_results['best_fitness']),
                    min(adaptive_results['best_fitness'])]
    
    improvements = [(s - a) / abs(s) * 100 for s, a in zip(standard_vals, adaptive_vals)]
    
    colors = ['green' if imp > 0 else 'red' for imp in improvements]
    axes[1, 1].bar(metrics, improvements, color=colors, alpha=0.7)
    axes[1, 1].set_title('性能改进百分比')
    axes[1, 1].set_ylabel('改进 (%)')
    axes[1, 1].axhline(y=0, color='black', linestyle='-', alpha=0.3)
    axes[1, 1].grid(True, alpha=0.3)
    
    plt.tight_layout()
    plt.savefig('5.问题五py转为cpp/optimizer_comparison.png', dpi=300, bbox_inches='tight')
    plt.show()
    
    print("📊 对比图表已保存为 optimizer_comparison.png")


if __name__ == "__main__":
    try:
        compare_performance()
    except KeyboardInterrupt:
        print("\n用户中断测试")
    except Exception as e:
        print(f"测试过程中出现错误: {e}")
        import traceback
        traceback.print_exc()
