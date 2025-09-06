# adaptive_de.py - 自适应差分进化算法实现
import numpy as np
import random
from collections import deque
from typing import Callable, List, Tuple, Optional
from enum import Enum


class MutationStrategy(Enum):
    """变异策略枚举"""
    DE_RAND_1 = "rand/1"
    DE_BEST_1 = "best/1"
    DE_CURRENT_TO_BEST_1 = "current-to-best/1"
    DE_RAND_2 = "rand/2"
    DE_BEST_2 = "best/2"


class BoundaryHandling(Enum):
    """边界处理策略枚举"""
    CLIP = "clip"
    REFLECT = "reflect"
    REINITIALIZE = "reinitialize"
    MIDPOINT = "midpoint"


class AdaptiveParameters:
    """自适应参数管理类"""
    
    def __init__(self, memory_size: int = 100):
        self.memory_size = memory_size
        self.successful_F = deque(maxlen=memory_size)
        self.successful_CR = deque(maxlen=memory_size)
        self.mean_F = 0.5
        self.mean_CR = 0.5
        self.std_F = 0.1
        self.std_CR = 0.1
        
    def add_success(self, F: float, CR: float):
        """添加成功的参数组合"""
        self.successful_F.append(F)
        self.successful_CR.append(CR)
        
    def update_parameters(self):
        """更新参数均值"""
        if len(self.successful_F) > 0:
            # 使用Lehmer均值计算F (更适合F参数的特性)
            F_values = np.array(self.successful_F)
            if np.sum(F_values) > 0:
                self.mean_F = np.sum(F_values ** 2) / np.sum(F_values)
            else:
                self.mean_F = 0.5
                
            # 使用算术均值计算CR
            self.mean_CR = np.mean(self.successful_CR)
            
            # 清空成功记录，准备下一代
            self.successful_F.clear()
            self.successful_CR.clear()
    
    def generate_parameters(self) -> Tuple[float, float]:
        """生成新的自适应参数"""
        # 使用正态分布生成F和CR
        F = np.clip(np.random.normal(self.mean_F, self.std_F), 0.0, 2.0)
        CR = np.clip(np.random.normal(self.mean_CR, self.std_CR), 0.0, 1.0)
        
        # 特殊处理：如果F太小，重新生成
        if F <= 0:
            F = np.random.uniform(0.1, 0.9)
            
        return F, CR


class StrategySelector:
    """策略选择器"""
    
    def __init__(self, strategies: List[MutationStrategy]):
        self.strategies = strategies
        self.success_rates = {strategy: 1.0 for strategy in strategies}
        self.total_uses = {strategy: 1 for strategy in strategies}
        self.recent_successes = {strategy: deque(maxlen=10) for strategy in strategies}
        
    def select_strategy(self) -> MutationStrategy:
        """根据成功率选择策略"""
        # 计算每个策略的加权概率
        weights = []
        for strategy in self.strategies:
            recent_success_rate = np.mean(self.recent_successes[strategy]) if self.recent_successes[strategy] else 0.5
            overall_rate = self.success_rates[strategy]
            # 综合考虑历史和近期表现
            weight = 0.7 * overall_rate + 0.3 * recent_success_rate
            weights.append(weight)
        
        # 归一化权重
        weights = np.array(weights)
        weights = weights / np.sum(weights)
        
        # 根据权重随机选择
        return np.random.choice(self.strategies, p=weights)
    
    def update_strategy_performance(self, strategy: MutationStrategy, success: bool):
        """更新策略性能"""
        self.total_uses[strategy] += 1
        self.recent_successes[strategy].append(1.0 if success else 0.0)
        
        # 更新总体成功率（带衰减）
        decay_factor = 0.95
        if success:
            self.success_rates[strategy] = decay_factor * self.success_rates[strategy] + (1 - decay_factor) * 1.0
        else:
            self.success_rates[strategy] = decay_factor * self.success_rates[strategy] + (1 - decay_factor) * 0.0


class AdaptiveDifferentialEvolution:
    """自适应差分进化算法 (JADE/SHADE风格)"""
    
    def __init__(self, 
                 objective_func: Callable[[np.ndarray], float],
                 bounds: List[Tuple[float, float]],
                 popsize: Optional[int] = None,
                 maxiter: int = 1000,
                 tol: float = 1e-6,
                 seed: Optional[int] = None,
                 boundary_handling: BoundaryHandling = BoundaryHandling.REFLECT,
                 use_archive: bool = True,
                 archive_size: int = 100,
                 adaptive_popsize: bool = False,
                 verbose: bool = False):
        
        self.objective_func = objective_func
        self.bounds = np.array(bounds)
        self.dim = len(bounds)
        self.lower_bounds = self.bounds[:, 0]
        self.upper_bounds = self.bounds[:, 1]
        
        # 算法参数
        self.popsize = popsize if popsize is not None else max(30, 4 * self.dim)
        self.maxiter = maxiter
        self.tol = tol
        self.boundary_handling = boundary_handling
        self.use_archive = use_archive
        self.archive_size = archive_size
        self.adaptive_popsize = adaptive_popsize
        self.verbose = verbose
        
        # 设置随机种子
        if seed is not None:
            np.random.seed(seed)
            random.seed(seed)
            
        # 自适应组件
        self.adaptive_params = AdaptiveParameters()
        available_strategies = [
            MutationStrategy.DE_RAND_1,
            MutationStrategy.DE_BEST_1,
            MutationStrategy.DE_CURRENT_TO_BEST_1,
            MutationStrategy.DE_RAND_2
        ]
        self.strategy_selector = StrategySelector(available_strategies)
        
        # 成功历史档案
        self.archive = deque(maxlen=archive_size) if use_archive else None
        
        # 算法状态
        self.population = None
        self.fitness = None
        self.best_idx = 0
        self.best_fitness = np.inf
        self.generation = 0
        self.convergence_history = []
        
    def initialize_population(self):
        """初始化种群"""
        self.population = np.random.uniform(
            self.lower_bounds, self.upper_bounds, 
            (self.popsize, self.dim)
        )
        
        # 评估初始种群
        self.fitness = np.array([self.objective_func(ind) for ind in self.population])
        
        # 找到最佳个体
        self.best_idx = np.argmin(self.fitness)
        self.best_fitness = self.fitness[self.best_idx]
        
        if self.verbose:
            print(f"初始化完成，种群大小: {self.popsize}, 维度: {self.dim}")
            print(f"初始最佳适应度: {self.best_fitness:.6f}")
    
    def mutate(self, target_idx: int, strategy: MutationStrategy, F: float) -> np.ndarray:
        """执行变异操作"""
        pop_size = len(self.population)
        
        if strategy == MutationStrategy.DE_RAND_1:
            # V = X_r1 + F * (X_r2 - X_r3)
            candidates = [i for i in range(pop_size) if i != target_idx]
            r1, r2, r3 = np.random.choice(candidates, 3, replace=False)
            mutant = self.population[r1] + F * (self.population[r2] - self.population[r3])
            
        elif strategy == MutationStrategy.DE_BEST_1:
            # V = X_best + F * (X_r1 - X_r2)
            candidates = [i for i in range(pop_size) if i != target_idx and i != self.best_idx]
            r1, r2 = np.random.choice(candidates, 2, replace=False)
            mutant = self.population[self.best_idx] + F * (self.population[r1] - self.population[r2])
            
        elif strategy == MutationStrategy.DE_CURRENT_TO_BEST_1:
            # V = X_i + F * (X_best - X_i) + F * (X_r1 - X_r2)
            candidates = [i for i in range(pop_size) if i != target_idx and i != self.best_idx]
            r1, r2 = np.random.choice(candidates, 2, replace=False)
            mutant = (self.population[target_idx] + 
                     F * (self.population[self.best_idx] - self.population[target_idx]) +
                     F * (self.population[r1] - self.population[r2]))
            
        elif strategy == MutationStrategy.DE_RAND_2:
            # V = X_r1 + F * (X_r2 - X_r3) + F * (X_r4 - X_r5)
            candidates = [i for i in range(pop_size) if i != target_idx]
            r1, r2, r3, r4, r5 = np.random.choice(candidates, 5, replace=False)
            mutant = (self.population[r1] + 
                     F * (self.population[r2] - self.population[r3]) +
                     F * (self.population[r4] - self.population[r5]))
            
        else:  # 默认使用DE_RAND_1
            candidates = [i for i in range(pop_size) if i != target_idx]
            r1, r2, r3 = np.random.choice(candidates, 3, replace=False)
            mutant = self.population[r1] + F * (self.population[r2] - self.population[r3])
            
        return mutant
    
    def crossover(self, target: np.ndarray, mutant: np.ndarray, CR: float) -> np.ndarray:
        """执行交叉操作"""
        trial = target.copy()
        
        # 确保至少有一个维度被交叉
        random_dim = np.random.randint(0, self.dim)
        
        for i in range(self.dim):
            if np.random.random() < CR or i == random_dim:
                trial[i] = mutant[i]
                
        return trial
    
    def handle_boundary(self, individual: np.ndarray) -> np.ndarray:
        """处理边界约束"""
        if self.boundary_handling == BoundaryHandling.CLIP:
            return np.clip(individual, self.lower_bounds, self.upper_bounds)
            
        elif self.boundary_handling == BoundaryHandling.REFLECT:
            for i in range(self.dim):
                if individual[i] < self.lower_bounds[i]:
                    individual[i] = self.lower_bounds[i] + (self.lower_bounds[i] - individual[i])
                    individual[i] = min(individual[i], self.upper_bounds[i])
                elif individual[i] > self.upper_bounds[i]:
                    individual[i] = self.upper_bounds[i] - (individual[i] - self.upper_bounds[i])
                    individual[i] = max(individual[i], self.lower_bounds[i])
            return individual
            
        elif self.boundary_handling == BoundaryHandling.REINITIALIZE:
            for i in range(self.dim):
                if individual[i] < self.lower_bounds[i] or individual[i] > self.upper_bounds[i]:
                    individual[i] = np.random.uniform(self.lower_bounds[i], self.upper_bounds[i])
            return individual
            
        elif self.boundary_handling == BoundaryHandling.MIDPOINT:
            for i in range(self.dim):
                if individual[i] < self.lower_bounds[i] or individual[i] > self.upper_bounds[i]:
                    individual[i] = (self.lower_bounds[i] + self.upper_bounds[i]) / 2.0
            return individual
            
        return individual
    
    def update_population_size(self):
        """动态调整种群大小 (LSHADE风格)"""
        if not self.adaptive_popsize:
            return
            
        # 线性缩减种群大小
        min_popsize = max(4, self.dim // 2)
        max_popsize = self.popsize
        
        progress = self.generation / self.maxiter
        current_popsize = int(max_popsize - progress * (max_popsize - min_popsize))
        current_popsize = max(current_popsize, min_popsize)
        
        if current_popsize < len(self.population):
            # 保留最优个体
            sorted_indices = np.argsort(self.fitness)
            self.population = self.population[sorted_indices[:current_popsize]]
            self.fitness = self.fitness[sorted_indices[:current_popsize]]
            self.best_idx = 0  # 重新设置最佳个体索引
            
    def optimize(self):
        """执行优化主循环"""
        # 初始化
        self.initialize_population()
        
        for generation in range(self.maxiter):
            self.generation = generation
            
            # 动态调整种群大小
            self.update_population_size()
            
            # 记录当前代的成功参数
            generation_successful_F = []
            generation_successful_CR = []
            generation_strategies_used = []
            generation_strategies_success = []
            
            # 进化每个个体
            for i in range(len(self.population)):
                # 生成自适应参数
                F, CR = self.adaptive_params.generate_parameters()
                
                # 选择变异策略
                strategy = self.strategy_selector.select_strategy()
                
                # 变异
                mutant = self.mutate(i, strategy, F)
                mutant = self.handle_boundary(mutant)
                
                # 交叉
                trial = self.crossover(self.population[i], mutant, CR)
                trial = self.handle_boundary(trial)
                
                # 评估试验个体
                trial_fitness = self.objective_func(trial)
                
                # 选择操作
                success = trial_fitness < self.fitness[i]
                if success:
                    # 记录成功的参数和策略
                    generation_successful_F.append(F)
                    generation_successful_CR.append(CR)
                    
                    # 将被替换的个体加入档案
                    if self.archive is not None:
                        self.archive.append(self.population[i].copy())
                    
                    # 替换个体
                    self.population[i] = trial
                    self.fitness[i] = trial_fitness
                    
                    # 更新全局最佳
                    if trial_fitness < self.best_fitness:
                        self.best_idx = i
                        self.best_fitness = trial_fitness
                
                # 记录策略使用情况
                generation_strategies_used.append(strategy)
                generation_strategies_success.append(success)
            
            # 更新自适应参数
            for F, CR in zip(generation_successful_F, generation_successful_CR):
                self.adaptive_params.add_success(F, CR)
            self.adaptive_params.update_parameters()
            
            # 更新策略选择器
            for strategy, success in zip(generation_strategies_used, generation_strategies_success):
                self.strategy_selector.update_strategy_performance(strategy, success)
            
            # 记录收敛历史
            self.convergence_history.append(self.best_fitness)
            
            # 打印进度
            if self.verbose and (generation % 50 == 0 or generation == self.maxiter - 1):
                print(f"代数 {generation}: 最佳适应度 = {self.best_fitness:.6f}, "
                      f"Mean F = {self.adaptive_params.mean_F:.3f}, Mean CR = {self.adaptive_params.mean_CR:.3f}")
            
            # 收敛检查
            if abs(self.best_fitness) < self.tol:
                if self.verbose:
                    print(f"在第 {generation} 代收敛")
                break
                
        return self.create_result()
    
    def create_result(self):
        """创建优化结果"""
        return {
            'x': self.population[self.best_idx].copy(),
            'fun': self.best_fitness,
            'nit': self.generation,
            'success': abs(self.best_fitness) < self.tol,
            'convergence_history': self.convergence_history,
            'final_population': self.population.copy(),
            'final_fitness': self.fitness.copy()
        }


def adaptive_differential_evolution(objective_func: Callable[[np.ndarray], float],
                                  bounds: List[Tuple[float, float]],
                                  **kwargs) -> dict:
    """
    自适应差分进化算法的便捷接口
    
    参数与scipy.optimize.differential_evolution兼容
    """
    # 提取参数
    popsize = kwargs.get('popsize', None)
    maxiter = kwargs.get('maxiter', 1000)
    tol = kwargs.get('tol', 1e-6)
    seed = kwargs.get('seed', None)
    disp = kwargs.get('disp', False)
    
    # 创建优化器
    optimizer = AdaptiveDifferentialEvolution(
        objective_func=objective_func,
        bounds=bounds,
        popsize=popsize,
        maxiter=maxiter,
        tol=tol,
        seed=seed,
        boundary_handling=BoundaryHandling.REFLECT,
        use_archive=True,
        adaptive_popsize=True,
        verbose=disp
    )
    
    # 执行优化
    return optimizer.optimize()


# 测试函数
if __name__ == "__main__":
    # 简单测试函数
    def sphere_function(x):
        return np.sum(x**2)
    
    # 测试优化
    bounds = [(-5.0, 5.0)] * 10
    result = adaptive_differential_evolution(
        sphere_function, 
        bounds, 
        maxiter=500, 
        disp=True
    )
    
    print(f"\n优化结果:")
    print(f"最优解: {result['x']}")
    print(f"最优值: {result['fun']}")
    print(f"迭代次数: {result['nit']}")
    print(f"成功: {result['success']}")
