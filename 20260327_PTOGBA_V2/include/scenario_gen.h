/**
 * @file scenario_gen.h
 * @brief 场景激励生成器
 * @author Hinton (Silver Maine Coon)
 * @date 2026-03-27
 *
 * 自主场景建模：海量碎片小组、混合并发、长尾掉队者、突发洪峰
 * 每一纳秒延迟都是犯罪。嗷唔。
 */

#ifndef SCENARIO_GEN_H
#define SCENARIO_GEN_H

#include <systemc>
#include <vector>
#include <random>
#include "types.h"
#include "sim_config.h"

/**
 * @class ScenarioGenerator
 * @brief 业务激励场景生成器
 */
SC_MODULE(ScenarioGenerator) {
    // ============================================================================
    // 端口 (Ports)
    // ============================================================================
    sc_core::sc_fifo_out<BarrierRequest> request_out;
    sc_core::sc_in<bool> clk;
    sc_core::sc_in<bool> reset;

    // ============================================================================
    // 配置参数 (Configuration)
    // ============================================================================
private:
    ScenarioType scenario_type_;
    uint32_t total_requests_;
    double burst_interval_ns_;
    unsigned int seed_;

    // 随机数生成器
    std::mt19937 rng_;
    std::uniform_int_distribution<uint16_t> task_id_dist_;
    std::uniform_int_distribution<uint16_t> priority_dist_;

    // ============================================================================
    // 核心方法 (Core Methods)
    // ============================================================================
public:
    // 生成线程
    void generate_process();

    // ============================================================================
    // 场景生成方法 (Scenario Generation Methods)
    // ============================================================================
private:
    /**
     * @brief V2.0 场景 A: MoE 训练常态
     * @return 请求列表
     */
    std::vector<BarrierRequest> generate_scenario_a();

    /**
     * @brief V2.0 场景 B: MoE 训练洪峰
     * @return 请求列表
     */
    std::vector<BarrierRequest> generate_scenario_b();

    /**
     * @brief V2.0 场景 C: 恶意压测
     * @return 请求列表
     */
    std::vector<BarrierRequest> generate_scenario_c();

    // V1.0 场景函数（保留但不再使用）
    std::vector<BarrierRequest> generate_fragmented_small();
    std::vector<BarrierRequest> generate_mixed_concurrent();
    std::vector<BarrierRequest> generate_tail_latency();
    std::vector<BarrierRequest> generate_burst_spike();
    std::vector<BarrierRequest> generate_all_scenarios();

    // ============================================================================
    // 辅助方法 (Helper Methods)
    // ============================================================================
private:
    /**
     * @brief 生成单个请求
     * @param task_id 任务 ID
     * @param group_size 组大小
     * @param priority 优先级
     * @return 请求
     */
    BarrierRequest make_request(uint16_t task_id, uint16_t group_size, uint16_t priority = 0);

public:
    // 构造函数
    SC_CTOR(ScenarioGenerator)
        : scenario_type_(ScenarioType::MIXED_CONCURRENT),
          total_requests_(1000),
          burst_interval_ns_(1000.0),
          seed_(RANDOM_SEED),
          task_id_dist_(1, 65535),
          priority_dist_(0, 1)
    {
        rng_.seed(seed_);

        SC_THREAD(generate_process);
        sensitive << clk.pos();
        async_reset_signal_is(reset, true);
    }

    // 配置场景类型
    void set_scenario(ScenarioType type) { scenario_type_ = type; }
    void set_total_requests(uint32_t n) { total_requests_ = n; }
    void set_burst_interval(double ns) { burst_interval_ns_ = ns; }
    void set_seed(unsigned int seed) { seed_ = seed; rng_.seed(seed); }
};

#endif // SCENARIO_GEN_H
