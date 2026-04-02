/**
 * @file scenario_gen.cpp
 * @brief 场景激励生成器实现
 * @author Hinton (Silver Maine Coon)
 * @date 2026-03-27
 *
 * 自主场景建模：海量碎片小组、混合并发、长尾掉队者、突发洪峰
 * 每一纳秒延迟都是犯罪。嗷唔。
 */

#include "scenario_gen.h"
#include <iostream>
#include <algorithm>

// ============================================================================
// 生成线程 (Generate Thread)
// ============================================================================
void ScenarioGenerator::generate_process() {
    // 等待复位释放
    while (reset.read()) {
        wait();
    }

    // 复位后等待一小段时间
    wait(100, sc_core::SC_NS);

    // 根据场景类型生成请求
    std::vector<BarrierRequest> requests;

    if (SCENARIO_FRAGMENTED_SMALL && SCENARIO_MIXED_CONCURRENT &&
        SCENARIO_TAIL_LATENCY && SCENARIO_BURST_SPIKE) {
        // 综合场景
        requests = generate_all_scenarios();
    } else {
        switch (scenario_type_) {
            case ScenarioType::FRAGMENTED_SMALL:
                requests = generate_fragmented_small();
                break;
            case ScenarioType::MIXED_CONCURRENT:
                requests = generate_mixed_concurrent();
                break;
            case ScenarioType::TAIL_LATENCY:
                requests = generate_tail_latency();
                break;
            case ScenarioType::BURST_SPIKE:
                requests = generate_burst_spike();
                break;
        }
    }

    // 打乱顺序，模拟真实并发
    std::shuffle(requests.begin(), requests.end(), rng_);

    // 发送请求
    uint32_t sent = 0;
    for (const auto& req : requests) {
        // 等待一小段时间（模拟请求到达间隔）
        wait(static_cast<double>(burst_interval_ns_), sc_core::SC_NS);

        // 检查复位
        if (reset.read()) {
            break;
        }

        request_out.write(req);
        sent++;

        // 每 100 个请求打印进度
        if (sent % 100 == 0) {
            std::cout << "[ScenarioGen] Sent " << sent << "/" << requests.size()
                      << " requests" << std::endl;
        }
    }

    std::cout << "[ScenarioGen] All " << sent << " requests sent." << std::endl;
}

// ============================================================================
// 场景1: 海量碎片小组
// ============================================================================
std::vector<BarrierRequest> ScenarioGenerator::generate_fragmented_small() {
    std::vector<BarrierRequest> requests;
    uint16_t task_id = 1;

    // 200 组 N=4
    for (int i = 0; i < 200; i++) {
        requests.push_back(make_request(task_id++, 4, 0));
    }

    // 50 组 N=8
    for (int i = 0; i < 50; i++) {
        requests.push_back(make_request(task_id++, 8, 0));
    }

    // 30 组 N=12
    for (int i = 0; i < 30; i++) {
        requests.push_back(make_request(task_id++, 12, 0));
    }

    std::cout << "[ScenarioGen] Fragmented Small: " << requests.size() << " requests" << std::endl;
    return requests;
}

// ============================================================================
// 场景2: 混合规模并发
// ============================================================================
std::vector<BarrierRequest> ScenarioGenerator::generate_mixed_concurrent() {
    std::vector<BarrierRequest> requests;
    uint16_t task_id = 1000;

    // 200 组 N=4 (碎片)
    for (int i = 0; i < 200; i++) {
        requests.push_back(make_request(task_id++, 4, 0));
    }

    // 50 组 N=16 (中等)
    for (int i = 0; i < 50; i++) {
        requests.push_back(make_request(task_id++, 16, 1));  // 高优先级
    }

    // 20 组 N=32 (中等)
    for (int i = 0; i < 20; i++) {
        requests.push_back(make_request(task_id++, 32, 1));
    }

    // 5 组 N=256 (大组，关键任务)
    for (int i = 0; i < 5; i++) {
        requests.push_back(make_request(task_id++, 256, 1));  // 高优先级
    }

    // 2 组 N=512 (超大组，必须走 GBA)
    for (int i = 0; i < 2; i++) {
        requests.push_back(make_request(task_id++, 512, 1));
    }

    std::cout << "[ScenarioGen] Mixed Concurrent: " << requests.size() << " requests" << std::endl;
    return requests;
}

// ============================================================================
// 场景3: 长尾掉队者
// ============================================================================
std::vector<BarrierRequest> ScenarioGenerator::generate_tail_latency() {
    std::vector<BarrierRequest> requests;
    uint16_t task_id = 2000;

    // 基础负载: 100 组正常请求
    for (int i = 0; i < 100; i++) {
        requests.push_back(make_request(task_id++, 16, 0));
    }

    // 长尾掉队者: 5% 的请求有更长超时（模拟掉队）
    std::uniform_int_distribution<int> tail_dist(0, 99);
    for (int i = 0; i < 20; i++) {
        BarrierRequest req = make_request(task_id++, 8, 0);
        // 10% 概率成为掉队者
        if (tail_dist(rng_) < 10) {
            req.timeout_ns = TIMEOUT_MS * 1000000 * 10;  // 10x 超时
        }
        requests.push_back(req);
    }

    std::cout << "[ScenarioGen] Tail Latency: " << requests.size() << " requests" << std::endl;
    return requests;
}

// ============================================================================
// 场景4: 突发洪峰
// ============================================================================
std::vector<BarrierRequest> ScenarioGenerator::generate_burst_spike() {
    std::vector<BarrierRequest> requests;
    uint16_t task_id = 3000;

    // 第一波: 正常负载
    for (int i = 0; i < 50; i++) {
        requests.push_back(make_request(task_id++, 16, 0));
    }

    // 第二波: 突发洪峰 - 500 组并发
    for (int i = 0; i < 500; i++) {
        uint16_t n = 4 + (i % 64);  // 组大小在 4-68 之间变化
        requests.push_back(make_request(task_id++, n, 0));
    }

    // 第三波: 恢复正常
    for (int i = 0; i < 50; i++) {
        requests.push_back(make_request(task_id++, 16, 0));
    }

    std::cout << "[ScenarioGen] Burst Spike: " << requests.size() << " requests" << std::endl;
    return requests;
}

// ============================================================================
// 综合场景 (所有场景叠加)
// ============================================================================
std::vector<BarrierRequest> ScenarioGenerator::generate_all_scenarios() {
    std::vector<BarrierRequest> requests;
    uint16_t task_id = 1;

    std::cout << "[ScenarioGen] Generating ALL scenarios..." << std::endl;

    // === 场景1: 海量碎片小组 ===
    if (SCENARIO_FRAGMENTED_SMALL) {
        // 200 组 N=4
        for (int i = 0; i < 200; i++) {
            requests.push_back(make_request(task_id++, 4, 0));
        }
        // 50 组 N=8
        for (int i = 0; i < 50; i++) {
            requests.push_back(make_request(task_id++, 8, 0));
        }
    }

    // === 场景2: 混合规模并发 ===
    if (SCENARIO_MIXED_CONCURRENT) {
        // 50 组 N=16 (中等)
        for (int i = 0; i < 50; i++) {
            requests.push_back(make_request(task_id++, 16, 1));
        }
        // 20 组 N=32
        for (int i = 0; i < 20; i++) {
            requests.push_back(make_request(task_id++, 32, 1));
        }
        // 10 组 N=64 (大组，关键任务)
        for (int i = 0; i < 10; i++) {
            requests.push_back(make_request(task_id++, 64, 1));
        }
        // 5 组 N=128
        for (int i = 0; i < 5; i++) {
            requests.push_back(make_request(task_id++, 128, 1));
        }
        // 3 组 N=256
        for (int i = 0; i < 3; i++) {
            requests.push_back(make_request(task_id++, 256, 1));
        }
    }

    // === 场景3: 长尾掉队者 ===
    if (SCENARIO_TAIL_LATENCY) {
        std::uniform_int_distribution<int> tail_dist(0, 99);
        for (int i = 0; i < 30; i++) {
            BarrierRequest req = make_request(task_id++, 8, 0);
            if (tail_dist(rng_) < 15) {  // 15% 掉队者
                req.timeout_ns = TIMEOUT_MS * 1000000 * 5;  // 5x 超时
            }
            requests.push_back(req);
        }
    }

    // === 场景4: 突发洪峰 ===
    if (SCENARIO_BURST_SPIKE) {
        for (int i = 0; i < 300; i++) {
            uint16_t n = 4 + (i % 60);  // 4-64
            requests.push_back(make_request(task_id++, n, 0));
        }
    }

    std::cout << "[ScenarioGen] ALL scenarios: " << requests.size() << " requests total" << std::endl;
    std::cout << "  - Fragmented Small: ~250" << std::endl;
    std::cout << "  - Mixed Concurrent: ~88" << std::endl;
    std::cout << "  - Tail Latency: ~30" << std::endl;
    std::cout << "  - Burst Spike: ~300" << std::endl;

    return requests;
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
