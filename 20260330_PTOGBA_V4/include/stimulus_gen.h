/**
 * @file stimulus_gen.h
 * @brief V4 时序化激励生成器
 * @author Hinton (Silver Maine Coon)
 * @date 2026-03-31
 *
 * 严格按照多普勒调研报告的激励方案实现。
 * 每一纳秒延迟都是犯罪。嗷唔。
 */

#ifndef STIMULUS_GEN_H
#define STIMULUS_GEN_H

#include <vector>
#include <map>
#include <cstdint>
#include <systemc>
#include "types.h"
#include "sim_config.h"

// ============================================================================
// 激励生成器
// ============================================================================
class StimulusGenerator {
public:
    /**
     * @brief 生成场景 A 激励 (Mixtral 8x7B Layer 16)
     * @return BarrierRequest 列表
     */
    static std::vector<BarrierRequest> generate_scenario_a();

    /**
     * @brief 生成场景 B 激励 (DeepSeek-V3 Layer 30)
     * @return BarrierRequest 列表
     */
    static std::vector<BarrierRequest> generate_scenario_b();

    /**
     * @brief 生成场景 C 激励 (DeepSeek-V3 Pipeline 4 层)
     * @return (触发时间, BarrierRequest) 列表
     */
    static std::vector<std::pair<sc_core::sc_time, BarrierRequest>> generate_scenario_c();

    /**
     * @brief 生成 Zipf 分布
     * @param num_items 项目数量
     * @param alpha Zipf 参数
     * @param total_tokens 总 token 数
     * @return 每个项目的 token 数分布
     */
    static std::vector<int> generate_zipf_distribution(
        int num_items,
        double alpha,
        int total_tokens
    );

    /**
     * @brief 根据场景类型生成激励
     * @param scenario 场景类型
     * @return BarrierRequest 列表（场景 C 只返回第一批）
     */
    static std::vector<BarrierRequest> generate(ScenarioType scenario);

    /**
     * @brief 获取场景的 member 分布
     * @param scenario 场景类型
     * @return member 数分布
     */
    static std::vector<int> get_member_distribution(ScenarioType scenario);

    /**
     * @brief 获取预期路径分布
     * @param scenario 场景类型
     * @return (路径类型, 任务数) 映射
     */
    static std::map<PathType, int> get_expected_path_distribution(ScenarioType scenario);

    /**
     * @brief 生成 Zipf 分布的 member 数
     * @param alpha Zipf 参数
     * @param min_n 最小 N
     * @param max_n 最大 N
     * @return Zipf 分布的随机 member 数
     */
    static int generate_zipf_member(double alpha, int min_n, int max_n);

private:
    /**
     * @brief 生成 Zipf 分布的随机索引
     * @param alpha Zipf 参数
     * @param n 项目数量
     * @return Zipf 分布的随机索引
     */
    static int zipf_random(double alpha, int n);

    // 随机数种子
    static unsigned int seed_;
};

#endif // STIMULUS_GEN_H
