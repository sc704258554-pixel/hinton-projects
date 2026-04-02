/**
 * @file stimulus_gen.cpp
 * @brief V4 时序化激励生成器实现
 * @author Hinton (Silver Maine Coon)
 * @date 2026-03-31
 *
 * 严格按照多普勒调研报告实现激励场景。
 * 每一纳秒延迟都是犯罪。嗷唔。
 */

#include "stimulus_gen.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>

unsigned int StimulusGenerator::seed_ = 42;

// ============================================================================
// 场景 A: Mixtral 8x7B Layer 16
// ============================================================================
std::vector<BarrierRequest> StimulusGenerator::generate_scenario_a() {
    std::vector<BarrierRequest> requests;

    // 场景 A 配置
    // t = 0 ns: 并发 8 个任务
    // Member 分布: [140, 135, 130, 128, 125, 122, 120, 124]
    const int member_dist[] = {140, 135, 130, 128, 125, 122, 120, 124};
    const int num_tasks = 8;

    for (int i = 0; i < num_tasks; ++i) {
        BarrierRequest req;
        req.task_id = i + 1;
        req.group_size = member_dist[i];  // N = member 数
        req.priority = 0;  // 普通优先级
        req.wave_id = 0;   // 同一波次
        req.arrival_time = sc_core::sc_time_stamp();  // t=0

        requests.push_back(req);
    }

    return requests;
}

// ============================================================================
// 场景 B: DeepSeek-V3 Layer 30
// ============================================================================
std::vector<BarrierRequest> StimulusGenerator::generate_scenario_b() {
    std::vector<BarrierRequest> requests;

    // 场景 B 配置
    // t = 0 ns: 并发 256 个任务
    // Member 分布: Zipf(α=1.0), N 范围 5-100
    const int num_tasks = 256;
    const int min_n = 5;
    const int max_n = 100;
    const double zipf_alpha = 1.0;

    // 生成 Zipf 分布的 member 数
    for (int i = 0; i < num_tasks; ++i) {
        BarrierRequest req;
        req.task_id = i + 1;

        // 使用 Zipf 分布生成 N
        int n = generate_zipf_member(zipf_alpha, min_n, max_n);
        req.group_size = n;

        req.priority = 0;
        req.wave_id = 0;
        req.arrival_time = sc_core::sc_time_stamp();

        requests.push_back(req);
    }

    return requests;
}

// ============================================================================
// 场景 C: DeepSeek-V3 Pipeline 4 层
// ============================================================================
std::vector<std::pair<sc_core::sc_time, BarrierRequest>>
StimulusGenerator::generate_scenario_c() {
    std::vector<std::pair<sc_core::sc_time, BarrierRequest>> requests;

    // 场景 C 配置
    // t = 0, 100, 200, 300 ns 各触发 256 个任务
    const int layers[] = {15, 30, 45, 60};
    const int num_layers = 4;
    const int tasks_per_layer = 256;
    const int min_n = 5;
    const int max_n = 100;
    const double zipf_alpha = 1.0;

    int task_id = 1;

    for (int layer_idx = 0; layer_idx < num_layers; ++layer_idx) {
        sc_core::sc_time trigger_time((double)(layer_idx * 100), sc_core::SC_NS);

        for (int i = 0; i < tasks_per_layer; ++i) {
            BarrierRequest req;
            req.task_id = task_id++;

            int n = generate_zipf_member(zipf_alpha, min_n, max_n);
            req.group_size = n;

            req.priority = 0;
            req.wave_id = layer_idx;  // 每层一个 wave_id
            req.arrival_time = trigger_time;

            requests.push_back({trigger_time, req});
        }
    }

    return requests;
}

// ============================================================================
// Zipf 分布生成
// ============================================================================
int StimulusGenerator::generate_zipf_member(double alpha, int min_n, int max_n) {
    // Zipf 分布生成
    // P(k) ∝ 1 / k^alpha

    // 使用拒绝采样生成 Zipf 分布
    double u, v;
    int k;

    do {
        u = static_cast<double>(rand_r(&seed_)) / RAND_MAX;
        v = static_cast<double>(rand_r(&seed_)) / RAND_MAX;

        // 生成 Zipf 随机数
        k = static_cast<int>(std::floor(std::pow(v, -1.0 / alpha)));

        // 限制范围
        if (k < min_n) k = min_n;
        if (k > max_n) k = max_n;

    } while (u > std::pow(static_cast<double>(min_n) / k, alpha));

    return k;
}

int StimulusGenerator::zipf_random(double alpha, int n) {
    // 生成 Zipf 分布的随机索引 [0, n-1]
    double u = static_cast<double>(rand_r(&seed_)) / RAND_MAX;

    double sum = 0.0;
    for (int i = 1; i <= n; ++i) {
        sum += 1.0 / std::pow(i, alpha);
    }

    double c = 1.0 / sum;  // 归一化常数

    double target = u / c;
    double cumulative = 0.0;

    for (int i = 1; i <= n; ++i) {
        cumulative += 1.0 / std::pow(i, alpha);
        if (cumulative >= target) {
            return i - 1;
        }
    }

    return n - 1;
}

// ============================================================================
// 工具函数
// ============================================================================
std::vector<int> StimulusGenerator::get_member_distribution(
    ScenarioType scenario
) {
    std::vector<int> distribution;

    switch (scenario) {
        case ScenarioType::A_MIXTRAL_L16: {
            const int member_dist[] = {140, 135, 130, 128, 125, 122, 120, 124};
            for (int n : member_dist) {
                distribution.push_back(n);
            }
            break;
        }

        case ScenarioType::B_DEEPSEEK_L30: {
            // Zipf 分布，256 个任务
            const int num_tasks = 256;
            for (int i = 0; i < num_tasks; ++i) {
                int n = generate_zipf_member(1.0, 5, 100);
                distribution.push_back(n);
            }
            break;
        }

        case ScenarioType::C_DEEPSEEK_PIPELINE: {
            // 4 层，每层 256 个任务 = 1024 个任务
            const int total_tasks = 1024;
            for (int i = 0; i < total_tasks; ++i) {
                int n = generate_zipf_member(1.0, 5, 100);
                distribution.push_back(n);
            }
            break;
        }
    }

    return distribution;
}

std::map<PathType, int> StimulusGenerator::get_expected_path_distribution(
    ScenarioType scenario
) {
    std::map<PathType, int> distribution;

    switch (scenario) {
        case ScenarioType::A_MIXTRAL_L16:
            // 所有任务 N >= 120，全部走 GBA
            distribution[PathType::GBA] = 8;
            distribution[PathType::MC] = 0;
            distribution[PathType::HOST] = 0;
            break;

        case ScenarioType::B_DEEPSEEK_L30:
            // 预期：GBA 96, MC 128, HOST 32
            distribution[PathType::GBA] = 96;
            distribution[PathType::MC] = 128;
            distribution[PathType::HOST] = 32;
            break;

        case ScenarioType::C_DEEPSEEK_PIPELINE:
            // 预期：GBA 320, MC 512, HOST 192
            distribution[PathType::GBA] = 320;
            distribution[PathType::MC] = 512;
            distribution[PathType::HOST] = 192;
            break;
    }

    return distribution;
}
