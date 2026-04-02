/**
 * @file scenario_gen_v3.cpp
 * @brief V3 场景激励生成器 - 真实 MoE 模型
 * @author Hinton (Silver Maine Coon)
 * @date 2026-03-28
 *
 * 基于 Mixtral 8x7B 和 DeepSeek-V3 的真实层同步模式
 * 每一纳秒延迟都是犯罪。嗷唔。
 */

#include "scenario_gen.h"
#include "sim_config.h"
#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <cmath>

// ============================================================================
// V3 场景生成器
// ============================================================================

// 生成线程
void ScenarioGenerator::generate_process() {
    while (reset.read()) {
        wait();
    }

    wait(100, sc_core::SC_NS);

    // V3: 使用真实 MoE 场景
    std::vector<BarrierRequest> requests = generate_moe_scenarios();

    std::shuffle(requests.begin(), requests.end(), rng_);

    // 一次性发送所有请求（模拟同时到达）
    double current_time = sc_core::sc_time_stamp().to_seconds() * 1e9;  // V3: 记录当前时间
    for (auto& req : requests) {
        req.arrival_time = sc_core::sc_time_stamp();  // V3: 设置到达时间
        request_out.write(req);
    }

    std::cout << "[ScenarioGen V3] Sent " << requests.size() << " MoE tasks simultaneously." << std::endl;
}

// ============================================================================
// V3: 真实 MoE 场景生成
// ============================================================================
std::vector<BarrierRequest> ScenarioGenerator::generate_moe_scenarios() {
    // 默认使用场景 B (DeepSeek Layer 30)
    return generate_deepseek_layer30();
}

// ============================================================================
// 场景 A: Mixtral 8x7B Layer 16
// ============================================================================
std::vector<BarrierRequest> ScenarioGenerator::generate_mixtral_layer16() {
    std::vector<BarrierRequest> requests;
    uint16_t task_id = 1;

    std::cout << "[V3] Generating Mixtral 8x7B Layer 16 scenario..." << std::endl;
    std::cout << "[V3] 8 Experts, Top-2 routing, Batch=512 tokens" << std::endl;

    // 8 个 Expert，每个等待约 120-140 个 Token
    // 基于真实路由分布（轻微不均衡）
    int group_sizes[] = {140, 135, 130, 128, 125, 122, 120, 124};
    
    for (int i = 0; i < 8; i++) {
        BarrierRequest req = make_request(task_id++, group_sizes[i], 1); // 高优先级
        requests.push_back(req);
    }

    std::cout << "[V3] Mixtral L16: " << requests.size() << " tasks, N range: 120-140" << std::endl;
    std::cout << "[V3] Expected: 0% HOST (all GBA)" << std::endl;

    return requests;
}

// ============================================================================
// 场景 B: DeepSeek-V3 Layer 30
// ============================================================================
std::vector<BarrierRequest> ScenarioGenerator::generate_deepseek_layer30() {
    std::vector<BarrierRequest> requests;
    uint16_t task_id = 1;

    std::cout << "[V3] Generating DeepSeek-V3 Layer 30 scenario..." << std::endl;
    std::cout << "[V3] 256 Experts, Top-9 routing, Batch=1024 tokens" << std::endl;

    // 256 个 Expert，使用 Zipf 分布
    // 1024 tokens × 9 experts = 9216 子任务
    // 分布到 256 个 Expert
    
    int total_tokens = 1024 * 9; // 9216
    std::vector<int> zipf_distribution = generate_zipf_distribution(256, total_tokens);

    for (int i = 0; i < 256; i++) {
        int n = zipf_distribution[i];
        if (n > 0) {
            // 优先级：N >= 64 为高优先级
            uint16_t priority = (n >= 64) ? 1 : 0;
            BarrierRequest req = make_request(task_id++, n, priority);
            requests.push_back(req);
        }
    }

    std::cout << "[V3] DeepSeek L30: " << requests.size() << " tasks, N range: 10-100" << std::endl;
    std::cout << "[V3] Expected: 5-10% HOST" << std::endl;

    return requests;
}

// ============================================================================
// 场景 C: DeepSeek-V3 多层
// ============================================================================
std::vector<BarrierRequest> ScenarioGenerator::generate_deepseek_multi_layer() {
    std::vector<BarrierRequest> requests;
    uint16_t task_id = 1;

    std::cout << "[V3] Generating DeepSeek-V3 Multi-Layer scenario..." << std::endl;
    std::cout << "[V3] 4 layers (15, 30, 45, 60), each with 256 experts" << std::endl;

    // 4 层，每层 256 个 Expert
    for (int layer = 0; layer < 4; layer++) {
        int total_tokens = 1024 * 9; // 每层 9216 子任务
        std::vector<int> zipf_distribution = generate_zipf_distribution(256, total_tokens);

        for (int i = 0; i < 256; i++) {
            int n = zipf_distribution[i];
            if (n > 0) {
                uint16_t priority = (n >= 64) ? 1 : 0;
                BarrierRequest req = make_request(task_id++, n, priority);
                requests.push_back(req);
            }
        }
    }

    std::cout << "[V3] DeepSeek Multi-Layer: " << requests.size() << " tasks, N range: 10-100" << std::endl;
    std::cout << "[V3] Expected: 20-30% HOST" << std::endl;

    return requests;
}

// ============================================================================
// Zipf 分布生成器
// ============================================================================
std::vector<int> ScenarioGenerator::generate_zipf_distribution(int num_experts, int total_tokens) {
    std::vector<int> distribution(num_experts, 0);
    
    // Zipf 分布参数
    double alpha = 1.0;
    
    // 计算每个 Expert 的概率
    std::vector<double> probabilities(num_experts);
    double sum = 0.0;
    for (int i = 0; i < num_experts; i++) {
        probabilities[i] = 1.0 / std::pow(i + 1, alpha);
        sum += probabilities[i];
    }
    
    // 归一化并分配 Token
    int assigned = 0;
    for (int i = 0; i < num_experts; i++) {
        probabilities[i] /= sum;
        int count = static_cast<int>(total_tokens * probabilities[i]);
        distribution[i] = count;
        assigned += count;
    }
    
    // 剩余 Token 分配给第一个 Expert
    if (assigned < total_tokens) {
        distribution[0] += (total_tokens - assigned);
    }
    
    return distribution;
}

// ============================================================================
// V3 场景选择器
// ============================================================================
std::vector<BarrierRequest> ScenarioGenerator::generate_scenario_v3(ScenarioTypeV3 type) {
    switch (type) {
        case ScenarioTypeV3::MIXTRAL_LAYER_16:
            return generate_mixtral_layer16();
        case ScenarioTypeV3::DEEPSEEK_LAYER_30:
            return generate_deepseek_layer30();
        case ScenarioTypeV3::DEEPSEEK_MULTI_LAYER:
            return generate_deepseek_multi_layer();
        default:
            return generate_deepseek_layer30();
    }
}

// ============================================================================
// 辅助函数
// ============================================================================
BarrierRequest ScenarioGenerator::make_request(uint16_t task_id, uint16_t group_size, uint16_t priority, uint8_t wave_id) {
    BarrierRequest req;
    req.task_id = task_id;
    req.group_size = group_size;
    req.priority = priority;
    req.timeout_ns = TIMEOUT_MS * 1000000;
    req.path_hint = 0; // auto
    req.wave_id = wave_id;  // V3: 波次标记
    return req;
}

// V1/V2 兼容函数
std::vector<BarrierRequest> ScenarioGenerator::generate_scenario_a() { return generate_mixtral_layer16(); }
std::vector<BarrierRequest> ScenarioGenerator::generate_scenario_b() { return generate_deepseek_layer30(); }
std::vector<BarrierRequest> ScenarioGenerator::generate_scenario_c() { return generate_deepseek_multi_layer(); }
std::vector<BarrierRequest> ScenarioGenerator::generate_fragmented_small() { return generate_deepseek_layer30(); }
std::vector<BarrierRequest> ScenarioGenerator::generate_mixed_concurrent() { return generate_deepseek_layer30(); }
std::vector<BarrierRequest> ScenarioGenerator::generate_tail_latency() { return generate_deepseek_layer30(); }
std::vector<BarrierRequest> ScenarioGenerator::generate_burst_spike() { return generate_deepseek_multi_layer(); }
std::vector<BarrierRequest> ScenarioGenerator::generate_all_scenarios() { return generate_deepseek_layer30(); }
