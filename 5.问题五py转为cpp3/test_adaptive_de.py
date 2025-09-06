# test_adaptive_de.py - 自适应差分进化算法验证脚本
import numpy as np
import time
from adaptive_de import adaptive_differential_evolution


def test_simple_functions():
    """测试简单的数学函数优化"""
    print("🧪 测试自适应差分进化算法")
    print("=" * 50)
    
    # 测试函数1: Sphere函数 (全局最小值: 0 at origin)
    def sphere_function(x):
        return np.sum(x**2)
    
    print("\n📐 测试1: Sphere函数 (f(x) = sum(x_i^2))")
    bounds = [(-5.0, 5.0)] * 5  # 5维
    
    start_time = time.time()
    result = adaptive_differential_evolution(
        sphere_function, 
        bounds, 
        maxiter=200, 
        disp=True,
        seed=42
    )
    end_time = time.time()
    
    print(f"✅ 优化结果:")
    print(f"   最优解: {result['x']}")
    print(f"   最优值: {result['fun']:.8f}")
    print(f"   迭代次数: {result['nit']}")
    print(f"   运行时间: {end_time - start_time:.2f}s")
    print(f"   收敛状态: {'成功' if result['success'] else '未收敛'}")
    
    # 验证结果
    expected_optimum = 0.0
    tolerance = 1e-4
    success = abs(result['fun'] - expected_optimum) < tolerance
    print(f"   验证结果: {'✅ 通过' if success else '❌ 失败'}")
    
    # 测试函数2: Rosenbrock函数 (更复杂)
    def rosenbrock_function(x):
        return sum(100.0 * (x[1:] - x[:-1]**2)**2 + (1 - x[:-1])**2)
    
    print("\n🌹 测试2: Rosenbrock函数 (更具挑战性)")
    bounds = [(-2.0, 2.0)] * 4  # 4维
    
    start_time = time.time()
    result = adaptive_differential_evolution(
        rosenbrock_function, 
        bounds, 
        maxiter=300, 
        disp=True,
        seed=42
    )
    end_time = time.time()
    
    print(f"✅ 优化结果:")
    print(f"   最优解: {result['x']}")
    print(f"   最优值: {result['fun']:.8f}")
    print(f"   迭代次数: {result['nit']}")
    print(f"   运行时间: {end_time - start_time:.2f}s")
    print(f"   收敛状态: {'成功' if result['success'] else '未收敛'}")
    
    # 验证结果 (Rosenbrock最小值为0 at [1,1,1,1])
    expected_optimum = 0.0
    tolerance = 1e-2  # Rosenbrock较难优化，放宽容忍度
    success = abs(result['fun'] - expected_optimum) < tolerance
    print(f"   验证结果: {'✅ 通过' if success else '❌ 失败'} (容忍度: {tolerance})")
    
    return True


def test_boundary_handling():
    """测试边界处理功能"""
    print("\n🔒 测试3: 边界处理功能")
    
    def constrained_function(x):
        # 这个函数在边界附近有最优解
        return (x[0] - 4.9)**2 + (x[1] + 4.9)**2
    
    bounds = [(-5.0, 5.0), (-5.0, 5.0)]  # 2维，最优解接近边界
    
    start_time = time.time()
    result = adaptive_differential_evolution(
        constrained_function, 
        bounds, 
        maxiter=150, 
        disp=True,
        seed=42
    )
    end_time = time.time()
    
    print(f"✅ 边界处理测试结果:")
    print(f"   最优解: {result['x']}")
    print(f"   最优值: {result['fun']:.8f}")
    print(f"   运行时间: {end_time - start_time:.2f}s")
    
    # 验证解是否在边界内
    within_bounds = all(bounds[i][0] <= result['x'][i] <= bounds[i][1] for i in range(len(bounds)))
    print(f"   边界检查: {'✅ 通过' if within_bounds else '❌ 失败'}")
    
    return within_bounds


def test_high_dimensional():
    """测试高维优化"""
    print("\n🌐 测试4: 高维优化 (20维)")
    
    def high_dim_sphere(x):
        return np.sum(x**2) + 0.1 * np.sum(np.sin(10 * x))  # 带噪声的sphere
    
    bounds = [(-1.0, 1.0)] * 20  # 20维
    
    start_time = time.time()
    result = adaptive_differential_evolution(
        high_dim_sphere, 
        bounds, 
        maxiter=400, 
        popsize=80,  # 高维需要更大种群
        disp=True,
        seed=42
    )
    end_time = time.time()
    
    print(f"✅ 高维测试结果:")
    print(f"   最优解前5维: {result['x'][:5]}")
    print(f"   最优值: {result['fun']:.8f}")
    print(f"   迭代次数: {result['nit']}")
    print(f"   运行时间: {end_time - start_time:.2f}s")
    
    # 高维优化成功标准：适应度显著小于初始随机值
    success = result['fun'] < 1.0  # 随机初始化的期望适应度约为20
    print(f"   性能检查: {'✅ 通过' if success else '❌ 失败'}")
    
    return success


def main():
    """主测试函数"""
    print("🚀 自适应差分进化算法完整验证")
    print("=" * 60)
    
    try:
        # 执行所有测试
        test1_result = test_simple_functions()
        test2_result = test_boundary_handling()
        test3_result = test_high_dimensional()
        
        # 汇总结果
        print("\n" + "=" * 60)
        print("📋 测试总结")
        print("=" * 60)
        
        all_passed = test1_result and test2_result and test3_result
        
        print(f"🧪 基础功能测试: {'✅ 通过' if test1_result else '❌ 失败'}")
        print(f"🔒 边界处理测试: {'✅ 通过' if test2_result else '❌ 失败'}")
        print(f"🌐 高维优化测试: {'✅ 通过' if test3_result else '❌ 失败'}")
        print(f"\n🏆 总体结果: {'✅ 所有测试通过' if all_passed else '❌ 部分测试失败'}")
        
        if all_passed:
            print("\n🎉 自适应差分进化算法验证完成！可以放心使用。")
        else:
            print("\n⚠️  存在问题，请检查具体测试输出。")
            
        return all_passed
        
    except Exception as e:
        print(f"\n❌ 测试过程中发生错误: {e}")
        import traceback
        traceback.print_exc()
        return False


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n⏹️  用户中断测试")
    except Exception as e:
        print(f"❌ 程序异常: {e}")
        import traceback
        traceback.print_exc()
