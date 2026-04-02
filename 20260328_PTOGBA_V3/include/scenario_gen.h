/**
 * @file scenario_gen.h
 * @brief 场景激励生成器 V3
 * @author Hinton (Silver Maine Coon)
 * @date 2026-03-28
 *
 * V3: 真实 MoE 模型层同步激励
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
 * @brief V3 业务激励场景生成器
 */
SC_MODULE(ScenarioGenerator) {
    sc_core::sc_fifo_out<BarrierRequest> request_out;
    sc_core::sc_in<bool> clk;
    sc_core::sc_in<bool> reset;

private:
    ScenarioType scenario_type_;
    uint32_t total_requests_;
    double burst_interval_ns_;
    unsigned int seed_;
    std::mt19937 rng_;

public:
    void generate_process();

private:
    // V3: 真实 MoE 场景
    std::vector<BarrierRequest> generate_moe_scenarios();
    std::vector<BarrierRequest> generate_mixtral_layer16();
    std::vector<BarrierRequest> generate_deepseek_layer30();
    std::vector<BarrierRequest> generate_deepseek_multi_layer();
    std::vector<int> generate_zipf_distribution(int num_experts, int total_tokens);
    std::vector<BarrierRequest> generate_scenario_v3(ScenarioTypeV3 type);

    // V2 兼容
    std::vector<BarrierRequest> generate_scenario_a();
    std::vector<BarrierRequest> generate_scenario_b();
    std::vector<BarrierRequest> generate_scenario_c();
    std::vector<BarrierRequest> generate_fragmented_small();
    std::vector<BarrierRequest> generate_mixed_concurrent();
    std::vector<BarrierRequest> generate_tail_latency();
    std::vector<BarrierRequest> generate_burst_spike();
    std::vector<BarrierRequest> generate_all_scenarios();

    BarrierRequest make_request(uint16_t task_id, uint16_t group_size, uint16_t priority = 0, uint8_t wave_id = 0);

public:
    SC_CTOR(ScenarioGenerator)
        : scenario_type_(ScenarioType::MIXED_CONCURRENT),
          total_requests_(1000),
          burst_interval_ns_(1000.0),
          seed_(RANDOM_SEED)
    {
        rng_.seed(seed_);
        SC_THREAD(generate_process);
        sensitive << clk.pos();
        async_reset_signal_is(reset, true);
    }

    void set_scenario(ScenarioType type) { scenario_type_ = type; }
    void set_total_requests(uint32_t n) { total_requests_ = n; }
    void set_burst_interval(double ns) { burst_interval_ns_ = ns; }
    void set_seed(unsigned int seed) { seed_ = seed; rng_.seed(seed); }
};

#endif // SCENARIO_GEN_H
