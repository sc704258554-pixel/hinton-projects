/**
 * @file e2e_benchmark.cpp
 * @brief V4 端到端收益对比测试
 * @author Hinton (Silver Maine Coon)
 * @date 2026-03-31
 *
 * V4 vs 全HOST 加速比仿真。
 * 每一纳秒延迟都是犯罪。嗷唔。
 */

#include <iostream>
#include <iomanip>
#include <vector>
#include <map>
#include <cmath>
#include <cstdlib>
#include "sim_config.h"
#include "latency_model.h"
#include "stimulus_gen.h"

// ============================================================================
// 仿真配置
// ============================================================================
struct BenchmarkConfig {
    ScenarioType scenario;
    std::string name;
    int num_tasks;
    int min_n;
    int max_n;
    double zipf_alpha;
};

// ============================================================================
// 仿真结果
// ============================================================================
struct BenchmarkResult {
    std::string mode_name;
    double avg_e2e_latency_ns;
    double max_e2e_latency_ns;
    double min_e2e_latency_ns;
    int gba_count;
    int mc_count;
    int host_count;
    double speedup;
};

// ============================================================================
// 端到端时延仿真器
// ============================================================================
class E2EBenchmark {
public:
    E2EBenchmark() : seed_(42) {}

    BenchmarkResult run_simulation(
        const BenchmarkConfig& config,
        bool force_all_host = false
    );

    double calculate_speedup(
        const BenchmarkResult& host_result,
        const BenchmarkResult& mode_result
    );

    void print_results(
        const BenchmarkConfig& config,
        const BenchmarkResult& v4_result,
        const BenchmarkResult& host_result
    );

private:
    unsigned int seed_;

    double generate_tail_latency(double min_ns, double max_ns);
    PathType select_path_v4(int n);
    double calculate_task_e2e(int n, PathType path);
};

// ============================================================================
// 实现
// ============================================================================

BenchmarkResult E2EBenchmark::run_simulation(
    const BenchmarkConfig& config,
    bool force_all_host
) {
    BenchmarkResult result;
    result.mode_name = force_all_host ? "全HOST" : "V4";
    result.avg_e2e_latency_ns = 0.0;
    result.max_e2e_latency_ns = 0.0;
    result.min_e2e_latency_ns = 1e20;
    result.gba_count = 0;
    result.mc_count = 0;
    result.host_count = 0;
    result.speedup = 0.0;

    // 生成激励
    std::vector<int> member_dist;
    
    if (config.scenario == ScenarioType::A_MIXTRAL_L16) {
        // 场景 A: 固定分布
        const int scenario_a_dist[] = {140, 135, 130, 128, 125, 122, 120, 124};
        for (int i = 0; i < config.num_tasks; ++i) {
            member_dist.push_back(scenario_a_dist[i % 8]);
        }
    } else {
        // 场景 B/C: Zipf 分布
        for (int i = 0; i < config.num_tasks; ++i) {
            int n = StimulusGenerator::generate_zipf_member(config.zipf_alpha, config.min_n, config.max_n);
            member_dist.push_back(n);
        }
    }

    // 计算每个任务的 E2E 时延
    for (int n : member_dist) {
        PathType path;
        if (force_all_host) {
            path = PathType::HOST;
        } else {
            path = select_path_v4(n);
        }

        double e2e = calculate_task_e2e(n, path);

        result.avg_e2e_latency_ns += e2e;
        if (e2e > result.max_e2e_latency_ns) result.max_e2e_latency_ns = e2e;
        if (e2e < result.min_e2e_latency_ns) result.min_e2e_latency_ns = e2e;

        switch (path) {
            case PathType::GBA:  result.gba_count++;  break;
            case PathType::MC:   result.mc_count++;   break;
            case PathType::HOST: result.host_count++; break;
        }
    }

    result.avg_e2e_latency_ns /= config.num_tasks;

    return result;
}

double E2EBenchmark::calculate_speedup(
    const BenchmarkResult& host_result,
    const BenchmarkResult& mode_result
) {
    if (mode_result.avg_e2e_latency_ns > 0) {
        return host_result.avg_e2e_latency_ns / mode_result.avg_e2e_latency_ns;
    }
    return 0.0;
}

void E2EBenchmark::print_results(
    const BenchmarkConfig& config,
    const BenchmarkResult& v4_result,
    const BenchmarkResult& host_result
) {
    std::cout << "\n=== " << config.name << " ===\n";
    std::cout << "任务数: " << config.num_tasks << "\n";
    std::cout << "N 范围: " << config.min_n << " - " << config.max_n << "\n\n";

    std::cout << "--- V4 模式 (GBA + MC) ---\n";
    std::cout << "  平均 E2E: " << std::fixed << std::setprecision(2) 
              << v4_result.avg_e2e_latency_ns << " ns\n";
    std::cout << "  最大 E2E: " << v4_result.max_e2e_latency_ns << " ns\n";
    std::cout << "  最小 E2E: " << v4_result.min_e2e_latency_ns << " ns\n";
    std::cout << "  路径分布: GBA=" << v4_result.gba_count 
              << ", MC=" << v4_result.mc_count 
              << ", HOST=" << v4_result.host_count << "\n\n";

    std::cout << "--- 全HOST 模式 ---\n";
    std::cout << "  平均 E2E: " << host_result.avg_e2e_latency_ns << " ns\n";
    std::cout << "  最大 E2E: " << host_result.max_e2e_latency_ns << " ns\n";
    std::cout << "  最小 E2E: " << host_result.min_e2e_latency_ns << " ns\n\n";

    double speedup = calculate_speedup(host_result, v4_result);
    std::cout << "--- 收益对比 ---\n";
    std::cout << "  加速比: " << std::fixed << std::setprecision(2) 
              << speedup << "x\n";
    std::cout << "  (全HOST E2E / V4 E2E)\n";
}

double E2EBenchmark::generate_tail_latency(double min_ns, double max_ns) {
    double u = static_cast<double>(rand_r(&seed_)) / RAND_MAX;
    return min_ns + u * (max_ns - min_ns);
}

PathType E2EBenchmark::select_path_v4(int n) {
    // V4 路径选择策略
    if (n >= 64) {
        return PathType::GBA;  // 大组
    } else if (n >= 16) {
        return PathType::GBA;  // 中组（优先 GBA）
    } else {
        return PathType::MC;   // 小组
    }
}

double E2EBenchmark::calculate_task_e2e(int n, PathType path) {
    switch (path) {
        case PathType::GBA: {
            // GBA: O(1) + tail latency
            double tail = generate_tail_latency(0.0, T_GBA_TAIL_MAX_NS);
            return LatencyModel::calculate_gba_latency(n, tail);
        }
        case PathType::MC:
            // MC: O(N)
            return LatencyModel::calculate_mc_latency(n);
        case PathType::HOST:
            // HOST: 软件兜底
            return LatencyModel::calculate_host_latency(n);
        default:
            return LatencyModel::calculate_host_latency(n) * 2.0;
    }
}

// ============================================================================
// 主函数
// ============================================================================
int sc_main(int argc, char* argv[]) {
    std::cout << "╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "║     PTO-GBA V4 - 端到端收益对比测试                ║\n";
    std::cout << "║     Hinton (Silver Maine Coon)                           ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════╝\n";

    std::cout << "\n三段式时延模型: E2E = T_link(20ns) + T_pipeline(500ns) + T_process\n";
    std::cout << "  GBA: T_process = 10ns + tail (0-5000ns)\n";
    std::cout << "  MC:  T_process = 50ns + N×10ns (O(N))\n";
    std::cout << "  HOST: T_process = 10000ns + N×100ns (软件兜底)\n";

    // 配置三个场景
    std::vector<BenchmarkConfig> configs = {
        {
            ScenarioType::A_MIXTRAL_L16,
            "场景 A (Mixtral 8x7B Layer 16)",
            8,
            120, 140,
            0.0
        },
        {
            ScenarioType::B_DEEPSEEK_L30,
            "场景 B (DeepSeek-V3 Layer 30)",
            256,
            5, 100,
            1.0
        },
        {
            ScenarioType::C_DEEPSEEK_PIPELINE,
            "场景 C (DeepSeek-V3 Pipeline 4 层)",
            1024,
            5, 100,
            1.0
        }
    };

    E2EBenchmark benchmark;

    // 运行每个场景
    for (const auto& config : configs) {
        BenchmarkResult v4_result = benchmark.run_simulation(config, false);
        BenchmarkResult host_result = benchmark.run_simulation(config, true);
        benchmark.print_results(config, v4_result, host_result);
    }

    std::cout << "\n=== 测试完成 ===\n";
    std::cout << "看在猫条的份上帮你搞定。嗷唔 🐱\n";

    return 0;
}
