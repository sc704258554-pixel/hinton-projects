/**
 * @file scenario_gen.cpp
 * @brief 场景激励生成器实现 V2.0
 * @author Hinton (Silver Maine Coon)
 * @date 2026-03-27
 *
 * V2.0: 极限激励场景 A/B/C
 * 每一纳秒延迟都是犯罪。嗷唔。
 */

#include "scenario_gen.h"
#include "sim_config.h"
#include <iostream>
#include <algorithm>
#include <cstdlib>

// ============================================================================
// 生成线程 (Generate Thread) - V2.1 批量突发模式
// ============================================================================
void ScenarioGenerator::generate_process() {
    // 等待复位释放
    while (reset.read()) {
        wait();
    }

    // 复位后等待一小段时间
    wait(100, sc_core::SC_NS);

    // V2.0: 直接生成场景 A (MoE 常态)
    std::vector<BarrierRequest> requests = generate_scenario_a();

    // 打乱顺序，模拟真实并发
    std::shuffle(requests.begin(), requests.end(), rng_);

    // V2.1: 批量突发发送 - 短时间内发送大量请求
    uint32_t sent = 0;
    uint32_t batch_count = 0;

    while (sent < requests.size()) {
        // 突发一批请求
        uint32_t batch_size = std::min(static_cast<uint32_t>(BURST_BATCH_SIZE), 
                                       static_cast<uint32_t>(requests.size() - sent));
        
        for (uint32_t i = 0; i < batch_size; i++) {
            request_out.write(requests[sent + i]);
        }
        sent += batch_size;
        batch_count++;

        std::cout << "[ScenarioGen V2.1] Burst batch #" << batch_count 
                  << ": sent " << batch_size << " requests (total: " << sent << ")" << std::endl;

        // 批次间短暂等待（模拟网络抖动）
        wait(BURST_ARRIVAL_INTERVAL_NS, sc_core::SC_NS);

        // 检查复位
        if (reset.read()) {
            break;
        }
    }

    std::cout << "[ScenarioGen V2.1] All " << sent << " requests sent in " 
              << batch_count << " burst batches." << std::endl;
}

// ============================================================================
// 生成单个请求
// ============================================================================
BarrierRequest ScenarioGenerator::make_request(
    uint16_t task_id,
    uint16_t group_size,
    uint16_t priority
) {
    BarrierRequest req;
    req.task_id = task_id;
    req.group_size = group_size;
    req.priority = priority;
    req.timeout_ns = TIMEOUT_MS * 1000000;  // ms -> ns
    req.path_hint = 0;  // auto
    return req;
}

// ============================================================================
// V2.0 场景 A: MoE 训练常态 (中等压力)
// ============================================================================
std::vector<BarrierRequest> ScenarioGenerator::generate_scenario_a() {
    std::vector<BarrierRequest> requests;
    uint16_t task_id = 1;
    uint16_t straggler_count = 0;

    std::cout << "[ScenarioGen V2.0] Generating Scenario A: MoE Normal..." << std::endl;

    // N=4: 500 个任务
    for (int i = 0; i < 500; i++) {
        BarrierRequest req = make_request(task_id++, 4, 0);
        // 10% 概率成为 Straggler
        if (rand() % 100 < 10) {
            req.timeout_ns = static_cast<uint32_t>(SCENARIO_A_STRAGGLER_DELAY_MS * 1000000);
            straggler_count++;
        }
        requests.push_back(req);
    }

    // N=16: 800 个任务
    for (int i = 0; i < 800; i++) {
        BarrierRequest req = make_request(task_id++, 16, rand() % 2);
        if (rand() % 100 < 10) {
            req.timeout_ns = static_cast<uint32_t>(SCENARIO_A_STRAGGLER_DELAY_MS * 1000000);
            straggler_count++;
        }
        requests.push_back(req);
    }

    // N=64: 500 个任务 (大组，高优先级)
    for (int i = 0; i < 500; i++) {
        BarrierRequest req = make_request(task_id++, 64, 1);
        if (rand() % 100 < 10) {
            req.timeout_ns = static_cast<uint32_t>(SCENARIO_A_STRAGGLER_DELAY_MS * 1000000);
            straggler_count++;
        }
        requests.push_back(req);
    }

    // N=256: 200 个任务 (超大组，高优先级)
    for (int i = 0; i < 200; i++) {
        BarrierRequest req = make_request(task_id++, 256, 1);
        if (rand() % 100 < 10) {
            req.timeout_ns = static_cast<uint32_t>(SCENARIO_A_STRAGGLER_DELAY_MS * 1000000);
            straggler_count++;
        }
        requests.push_back(req);
    }

    std::cout << "[ScenarioGen V2.0] Scenario A: " << requests.size() 
              << " requests, Stragglers: " << straggler_count 
              << " (" << (100.0 * straggler_count / requests.size()) << "%)" << std::endl;
    std::cout << "  Expected software fallback: 10-15%" << std::endl;

    return requests;
}

// ============================================================================
// V2.0 场景 B: MoE 训练洪峰 (极限压力)
// ============================================================================
std::vector<BarrierRequest> ScenarioGenerator::generate_scenario_b() {
    std::vector<BarrierRequest> requests;
    uint16_t task_id = 1;
    uint16_t straggler_count = 0;

    std::cout << "[ScenarioGen V2.0] Generating Scenario B: MoE Burst..." << std::endl;

    // N=4: 2000 个任务 (大量碎片)
    for (int i = 0; i < 2000; i++) {
        BarrierRequest req = make_request(task_id++, 4, 0);
        // 20% 概率成为 Straggler
        if (rand() % 100 < 20) {
            req.timeout_ns = static_cast<uint32_t>(STRAGGLER_DELAY_MS * 1000000);
            straggler_count++;
        }
        requests.push_back(req);
    }

    // N=16: 1500 个任务
    for (int i = 0; i < 1500; i++) {
        BarrierRequest req = make_request(task_id++, 16, rand() % 2);
        if (rand() % 100 < 20) {
            req.timeout_ns = static_cast<uint32_t>(STRAGGLER_DELAY_MS * 1000000);
            straggler_count++;
        }
        requests.push_back(req);
    }

    // N=64: 1000 个任务
    for (int i = 0; i < 1000; i++) {
        BarrierRequest req = make_request(task_id++, 64, 1);
        if (rand() % 100 < 20) {
            req.timeout_ns = static_cast<uint32_t>(STRAGGLER_DELAY_MS * 1000000);
            straggler_count++;
        }
        requests.push_back(req);
    }

    // N=256: 500 个任务
    for (int i = 0; i < 500; i++) {
        BarrierRequest req = make_request(task_id++, 256, 1);
        if (rand() % 100 < 20) {
            req.timeout_ns = static_cast<uint32_t>(STRAGGLER_DELAY_MS * 1000000);
            straggler_count++;
        }
        requests.push_back(req);
    }

    std::cout << "[ScenarioGen V2.0] Scenario B: " << requests.size() 
              << " requests, Stragglers: " << straggler_count 
              << " (" << (100.0 * straggler_count / requests.size()) << "%)" << std::endl;
    std::cout << "  Expected software fallback: 40-50%" << std::endl;

    return requests;
}

// ============================================================================
// V2.0 场景 C: 恶意压测 (压力测试)
// ============================================================================
std::vector<BarrierRequest> ScenarioGenerator::generate_scenario_c() {
    std::vector<BarrierRequest> requests;
    uint16_t task_id = 1;
    uint16_t straggler_count = 0;

    std::cout << "[ScenarioGen V2.0] Generating Scenario C: Malicious Stress Test..." << std::endl;

    // 全部 N=4: 10000 个任务
    for (int i = 0; i < SCENARIO_C_TOTAL_TASKS; i++) {
        BarrierRequest req = make_request(task_id++, 4, 0);
        // 30% 概率成为 Straggler
        if (rand() % 100 < 30) {
            req.timeout_ns = static_cast<uint32_t>(STRAGGLER_DELAY_MS * 1000000);
            straggler_count++;
        }
        requests.push_back(req);
    }

    std::cout << "[ScenarioGen V2.0] Scenario C: " << requests.size() 
              << " requests, Stragglers: " << straggler_count 
              << " (" << (100.0 * straggler_count / requests.size()) << "%)" << std::endl;
    std::cout << "  Expected software fallback: > 70%" << std::endl;

    return requests;
}

// V1.0 兼容函数（重定向到 V2.0）
std::vector<BarrierRequest> ScenarioGenerator::generate_fragmented_small() {
    return generate_scenario_a();
}

std::vector<BarrierRequest> ScenarioGenerator::generate_mixed_concurrent() {
    return generate_scenario_a();
}

std::vector<BarrierRequest> ScenarioGenerator::generate_tail_latency() {
    return generate_scenario_a();
}

std::vector<BarrierRequest> ScenarioGenerator::generate_burst_spike() {
    return generate_scenario_b();
}

std::vector<BarrierRequest> ScenarioGenerator::generate_all_scenarios() {
    return generate_scenario_a();
}
