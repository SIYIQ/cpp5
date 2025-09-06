#include "cpp_optimizer_wrapper.hpp"
#include <iostream>
#include <chrono>
#include <vector>
#include <random>
#include <cmath>

using namespace OptimizerWrapper;
using namespace HighPerformanceDE;

// 标准测试函数
namespace TestFunctions {

// Sphere函数 - 简单凸函数
double sphere(const Vector& x) {
    return x.squaredNorm();
}

// Rosenbrock函数 - 经典非凸函数  
double rosenbrock(const Vector& x) {
    double sum = 0.0;
    for (int i = 0; i < x.size() - 1; ++i) {
        double t1 = x[i+1] - x[i] * x[i];
        double t2 = 1.0 - x[i];
        sum += 100.0 * t1 * t1 + t2 * t2;
    }
    return sum;
}

// Rastrigin函数 - 多峰函数
double rastrigin(const Vector& x) {
    double sum = 10.0 * x.size();
    for (int i = 0; i < x.size(); ++i) {
        sum += x[i] * x[i] - 10.0 * std::cos(2.0 * M_PI * x[i]);
    }
    return sum;
}

// Schwefel函数 - 困难的多峰函数
double schwefel(const Vector& x) {
    double sum = 418.9829 * x.size();
    for (int i = 0; i < x.size(); ++i) {
        sum -= x[i] * std::sin(std::sqrt(std::abs(x[i])));
    }
    return sum;
}

} // namespace TestFunctions

struct BenchmarkResult {
    std::string function_name;
    int dimension;
    double avg_time;
    double avg_fitness;
    double std_fitness;
    double best_fitness;
    double success_rate;
    int avg_evaluations;
};

BenchmarkResult benchmark_function(
    const std::string& name,
    std::function<double(const Vector&)> func,
    const std::vector<std::pair<double, double>>& bounds,
    int num_runs = 10) {
    
    std::cout << "\n🔬 基准测试: " << name << " (" << bounds.size() << "维)" << std::endl;
    
    BenchmarkResult result;
    result.function_name = name;
    result.dimension = bounds.size();
    
    std::vector<double> times, fitnesses;
    std::vector<int> evaluations;
    int successes = 0;
    
    AdaptiveDESettings settings;
    settings.population_size = std::max(60, 4 * static_cast<int>(bounds.size()));
    settings.max_iterations = 500;
    settings.tolerance = 1e-6;
    settings.verbose = false;
    
    for (int run = 0; run < num_runs; ++run) {
        settings.random_seed = 42 + run;
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        auto opt_result = adaptive_differential_evolution(func, bounds, settings);
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        times.push_back(duration.count() / 1000.0);
        fitnesses.push_back(opt_result.best_fitness);
        evaluations.push_back(opt_result.performance_stats.total_evaluations);
        
        if (opt_result.converged || opt_result.best_fitness < 1e-4) {
            successes++;
        }
        
        std::cout << "  运行 " << (run + 1) << "/" << num_runs 
                  << ": f=" << std::scientific << std::setprecision(3) << opt_result.best_fitness
                  << ", t=" << std::fixed << std::setprecision(2) << (duration.count() / 1000.0) << "s"
                  << ", eval=" << opt_result.performance_stats.total_evaluations << std::endl;
    }
    
    // 计算统计信息
    result.avg_time = std::accumulate(times.begin(), times.end(), 0.0) / times.size();
    result.avg_fitness = std::accumulate(fitnesses.begin(), fitnesses.end(), 0.0) / fitnesses.size();
    result.best_fitness = *std::min_element(fitnesses.begin(), fitnesses.end());
    result.success_rate = static_cast<double>(successes) / num_runs;
    result.avg_evaluations = std::accumulate(evaluations.begin(), evaluations.end(), 0) / evaluations.size();
    
    // 计算标准差
    double variance = 0.0;
    for (double f : fitnesses) {
        variance += (f - result.avg_fitness) * (f - result.avg_fitness);
    }
    result.std_fitness = std::sqrt(variance / fitnesses.size());
    
    return result;
}

void test_scalability() {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "📈 可扩展性测试" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    std::vector<int> dimensions = {5, 10, 20, 30, 50};
    std::vector<BenchmarkResult> scalability_results;
    
    for (int dim : dimensions) {
        std::vector<std::pair<double, double>> bounds(dim, {-5.0, 5.0});
        
        auto result = benchmark_function(
            "Sphere_" + std::to_string(dim) + "D",
            TestFunctions::sphere,
            bounds,
            5 // 减少运行次数以节省时间
        );
        
        scalability_results.push_back(result);
    }
    
    std::cout << "\n📊 可扩展性结果总结:" << std::endl;
    std::cout << std::setw(8) << "维度" 
              << std::setw(12) << "平均时间(s)"
              << std::setw(12) << "平均适应度" 
              << std::setw(10) << "成功率(%)"
              << std::setw(12) << "平均评估" << std::endl;
    std::cout << std::string(54, '-') << std::endl;
    
    for (const auto& result : scalability_results) {
        std::cout << std::setw(8) << result.dimension
                  << std::setw(12) << std::fixed << std::setprecision(2) << result.avg_time
                  << std::setw(12) << std::scientific << std::setprecision(2) << result.avg_fitness
                  << std::setw(10) << std::fixed << std::setprecision(1) << (result.success_rate * 100)
                  << std::setw(12) << result.avg_evaluations << std::endl;
    }
}

void test_parallel_performance() {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "⚡ 并行性能测试" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    const int dimension = 30;
    std::vector<std::pair<double, double>> bounds(dimension, {-5.0, 5.0});
    
    std::vector<int> thread_counts = {1, 2, 4, 8};
    if (std::thread::hardware_concurrency() > 8) {
        thread_counts.push_back(std::thread::hardware_concurrency());
    }
    
    std::vector<double> execution_times;
    
    for (int num_threads : thread_counts) {
        std::cout << "\n测试线程数: " << num_threads << std::endl;
        
        AdaptiveDESettings settings;
        settings.population_size = 120;
        settings.max_iterations = 300;
        settings.tolerance = 1e-6;
        settings.verbose = false;
        settings.num_threads = num_threads;
        settings.random_seed = 42;
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        auto result = adaptive_differential_evolution(TestFunctions::rosenbrock, bounds, settings);
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        double exec_time = duration.count() / 1000.0;
        execution_times.push_back(exec_time);
        
        std::cout << "  执行时间: " << std::fixed << std::setprecision(2) << exec_time << "s" << std::endl;
        std::cout << "  最终适应度: " << std::scientific << std::setprecision(4) << result.best_fitness << std::endl;
    }
    
    // 计算加速比
    std::cout << "\n📊 并行性能总结:" << std::endl;
    std::cout << std::setw(8) << "线程数" 
              << std::setw(12) << "执行时间(s)"
              << std::setw(10) << "加速比" 
              << std::setw(12) << "效率(%)" << std::endl;
    std::cout << std::string(42, '-') << std::endl;
    
    double baseline_time = execution_times[0];
    
    for (size_t i = 0; i < thread_counts.size(); ++i) {
        double speedup = baseline_time / execution_times[i];
        double efficiency = (speedup / thread_counts[i]) * 100.0;
        
        std::cout << std::setw(8) << thread_counts[i]
                  << std::setw(12) << std::fixed << std::setprecision(2) << execution_times[i]
                  << std::setw(10) << std::setprecision(2) << speedup
                  << std::setw(12) << std::setprecision(1) << efficiency << std::endl;
    }
}

void test_difficult_functions() {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "🎯 困难函数测试" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    const int dimension = 20;
    std::vector<BenchmarkResult> results;
    
    // Sphere函数 (简单基准)
    {
        std::vector<std::pair<double, double>> bounds(dimension, {-5.0, 5.0});
        auto result = benchmark_function("Sphere", TestFunctions::sphere, bounds, 5);
        results.push_back(result);
    }
    
    // Rosenbrock函数 (经典困难函数)
    {
        std::vector<std::pair<double, double>> bounds(dimension, {-2.0, 2.0});
        auto result = benchmark_function("Rosenbrock", TestFunctions::rosenbrock, bounds, 5);
        results.push_back(result);
    }
    
    // Rastrigin函数 (多峰函数)
    {
        std::vector<std::pair<double, double>> bounds(dimension, {-5.12, 5.12});
        auto result = benchmark_function("Rastrigin", TestFunctions::rastrigin, bounds, 5);
        results.push_back(result);
    }
    
    // Schwefel函数 (极难函数)
    {
        std::vector<std::pair<double, double>> bounds(dimension, {-500.0, 500.0});
        auto result = benchmark_function("Schwefel", TestFunctions::schwefel, bounds, 5);
        results.push_back(result);
    }
    
    // 生成对比表格
    std::cout << "\n📊 困难函数测试总结:" << std::endl;
    std::cout << std::setw(12) << "函数" 
              << std::setw(12) << "平均时间(s)"
              << std::setw(15) << "平均适应度" 
              << std::setw(15) << "最佳适应度"
              << std::setw(10) << "成功率(%)" << std::endl;
    std::cout << std::string(64, '-') << std::endl;
    
    for (const auto& result : results) {
        std::cout << std::setw(12) << result.function_name
                  << std::setw(12) << std::fixed << std::setprecision(2) << result.avg_time
                  << std::setw(15) << std::scientific << std::setprecision(3) << result.avg_fitness
                  << std::setw(15) << result.best_fitness
                  << std::setw(10) << std::fixed << std::setprecision(1) << (result.success_rate * 100)
                  << std::endl;
    }
}

void generate_benchmark_report(const std::vector<BenchmarkResult>& results) {
    std::ofstream file("cpp_benchmark_report.html");
    if (!file.is_open()) {
        std::cerr << "无法创建基准测试报告" << std::endl;
        return;
    }
    
    file << R"(<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>C++高性能自适应DE基准测试报告</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; }
        table { border-collapse: collapse; width: 100%; margin: 20px 0; }
        th, td { border: 1px solid #ddd; padding: 12px; text-align: center; }
        th { background-color: #2E86AB; color: white; font-weight: bold; }
        tr:nth-child(even) { background-color: #f9f9f9; }
        .excellent { background-color: #90EE90; }
        .good { background-color: #FFE4B5; }
        .poor { background-color: #FFB6C1; }
        .metric { font-weight: bold; color: #2E86AB; }
        .summary { background-color: #f0f8ff; padding: 15px; margin: 20px 0; }
    </style>
</head>
<body>
    <h1>🚀 高性能自适应差分进化算法基准测试报告</h1>
    
    <div class="summary">
        <h2>📊 测试摘要</h2>
        <ul>
            <li><strong>测试函数数量:</strong> )" << results.size() << R"( 个</li>
            <li><strong>算法类型:</strong> 高性能自适应差分进化 (JADE/SHADE)</li>
            <li><strong>测试日期:</strong> )" << std::chrono::system_clock::now().time_since_epoch().count() << R"(</li>
        </ul>
    </div>
    
    <h2>🎯 基准测试结果</h2>
    <table>
        <tr>
            <th>测试函数</th>
            <th>维度</th>
            <th>平均时间 (s)</th>
            <th>平均适应度</th>
            <th>最佳适应度</th>
            <th>标准差</th>
            <th>成功率 (%)</th>
            <th>平均评估次数</th>
            <th>性能评级</th>
        </tr>
)";
    
    for (const auto& result : results) {
        std::string rating_class;
        std::string rating_text;
        
        if (result.success_rate > 0.8) {
            rating_class = "excellent";
            rating_text = "优秀";
        } else if (result.success_rate > 0.5) {
            rating_class = "good"; 
            rating_text = "良好";
        } else {
            rating_class = "poor";
            rating_text = "一般";
        }
        
        file << "        <tr>\n";
        file << "            <td>" << result.function_name << "</td>\n";
        file << "            <td>" << result.dimension << "</td>\n";
        file << "            <td>" << std::fixed << std::setprecision(2) << result.avg_time << "</td>\n";
        file << "            <td>" << std::scientific << std::setprecision(3) << result.avg_fitness << "</td>\n";
        file << "            <td>" << result.best_fitness << "</td>\n";
        file << "            <td>" << result.std_fitness << "</td>\n";
        file << "            <td>" << std::fixed << std::setprecision(1) << (result.success_rate * 100) << "</td>\n";
        file << "            <td>" << result.avg_evaluations << "</td>\n";
        file << "            <td class=\"" << rating_class << "\">" << rating_text << "</td>\n";
        file << "        </tr>\n";
    }
    
    file << R"(    </table>
    
    <h2>🔍 性能分析</h2>
    <div class="summary">
        <h3>算法优势</h3>
        <ul>
            <li><strong>参数自适应:</strong> F和CR参数自动调整，无需手工调优</li>
            <li><strong>多策略融合:</strong> 动态选择最优变异策略</li>
            <li><strong>并行优化:</strong> 充分利用多核处理器性能</li>
            <li><strong>内存高效:</strong> 优化的数据结构和缓存机制</li>
        </ul>
        
        <h3>适用场景</h3>
        <ul>
            <li>高维连续优化问题</li>
            <li>复杂约束优化</li>
            <li>计算昂贵的黑盒优化</li>
            <li>实时优化应用</li>
        </ul>
    </div>
    
</body>
</html>
)";
    
    file.close();
    std::cout << "\n📄 基准测试报告已保存到: cpp_benchmark_report.html" << std::endl;
}

int main() {
    try {
        std::cout << "🔬 高性能C++自适应差分进化算法基准测试" << std::endl;
        std::cout << std::string(60, '=') << std::endl;
        
        Utils::print_system_info();
        
        // 基础函数测试
        test_difficult_functions();
        
        // 可扩展性测试  
        test_scalability();
        
        // 并行性能测试
        test_parallel_performance();
        
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "✅ 基准测试完成！" << std::endl;
        std::cout << "报告已保存到 cpp_benchmark_report.html" << std::endl;
        std::cout << std::string(60, '=') << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ 基准测试失败: " << e.what() << std::endl;
        return 1;
    }
}
