/**
 * @file sim_config.h
 * @brief V4 三段式物理时延模型配置
 * @author Hinton (Silver Maine Coon)
 * @date 2026-03-31
 *
 * E2E_Latency = T_link + T_pipeline + T_process
 * 每一纳秒延迟都是犯罪。嗷唔。
 */

#ifndef SIM_CONFIG_H
#define SIM_CONFIG_H

#include <cstdint>
#include <cmath>

// ============================================================================
// V4 三段式物理时延模型
// ============================================================================

// ----------------------------------------------------------------------------
// 第一段：T_link（物理链路 RTT）
// ----------------------------------------------------------------------------
constexpr double FIBER_LENGTH_M = 2.0;                // 光纤长度（米）
constexpr double FIBER_REFRACTIVE_INDEX = 1.467;     // 石英玻璃折射率
constexpr double LIGHT_SPEED_M_PER_NS = 0.299792;    // 光速 m/ns
constexpr double T_LINK_NS = 2.0 * FIBER_LENGTH_M / (LIGHT_SPEED_M_PER_NS / FIBER_REFRACTIVE_INDEX);
// ≈ 20 ns

// ----------------------------------------------------------------------------
// 第二段：T_pipeline（流水线底噪）
// ----------------------------------------------------------------------------
// MAC → Parser → Crossbar → TM
constexpr double T_PIPELINE_MAC_NS = 50.0;      // 帧接收/发送
constexpr double T_PIPELINE_PARSER_NS = 100.0;  // 包头解析
constexpr double T_PIPELINE_CROSSBAR_NS = 200.0; // 交换矩阵
constexpr double T_PIPELINE_TM_NS = 150.0;      // 流量管理
constexpr double T_PIPELINE_NS = T_PIPELINE_MAC_NS + T_PIPELINE_PARSER_NS + 
                                 T_PIPELINE_CROSSBAR_NS + T_PIPELINE_TM_NS;
// = 500 ns

// ----------------------------------------------------------------------------
// 第三段：GBA 处理（O(1) 硬件极速 + 占坑时间）
// ----------------------------------------------------------------------------
constexpr double T_GBA_PURE_NS = 10.0;          // 纯处理：查表 + 状态机翻转
constexpr double T_GBA_TAIL_MAX_NS = 5000.0;    // 长尾掉队者上限
constexpr double T_GBA_TAIL_MIN_NS = 0.0;        // 长尾下限

// GBA 占坑时间 = 首尾到达时间差（动态计算）

// ----------------------------------------------------------------------------
// 第三段：MC 处理（O(N) 线性复制）
// ----------------------------------------------------------------------------
constexpr double T_MC_BASE_NS = 50.0;           // 微码引擎启动
constexpr double T_MC_COPY_PER_MEMBER_NS = 10.0; // 每个 member 的复制惩罚

// ----------------------------------------------------------------------------
// 端到端时延计算函数
// ----------------------------------------------------------------------------

/**
 * @brief 计算 GBA 路径端到端时延
 * @param N 组大小（member 数量）
 * @param tail_latency_ns 首尾到达时间差（占坑时间）
 * @return 端到端时延（ns）
 */
inline double calculate_e2e_latency_gba(int N, double tail_latency_ns) {
    double T_process = T_GBA_PURE_NS + tail_latency_ns;
    return T_LINK_NS + T_PIPELINE_NS + T_process;
}

/**
 * @brief 计算 MC 路径端到端时延
 * @param N 组大小（member 数量）
 * @return 端到端时延（ns）
 */
inline double calculate_e2e_latency_mc(int N) {
    double T_process = T_MC_BASE_NS + N * T_MC_COPY_PER_MEMBER_NS;
    return T_LINK_NS + T_PIPELINE_NS + T_process;
}

/**
 * @brief 计算 HOST 路径端到端时延（软件兜底）
 * @param N 组大小
 * @return 端到端时延（ns）- 对数正态长尾
 */
inline double calculate_e2e_latency_host(int N) {
    // 软件兜底：固定高时延 + 长尾抖动
    constexpr double T_HOST_BASE_NS = 10000.0;  // 10 μs 基础
    constexpr double T_HOST_PER_MEMBER_NS = 100.0; // 每个 member
    return T_HOST_BASE_NS + N * T_HOST_PER_MEMBER_NS;
}

// ============================================================================
// 资源配置
// ============================================================================

// GBA Slots
constexpr uint32_t GBA_TOTAL_SLOTS = 512;

// MC Outstanding
constexpr uint32_t MC_MAX_OUTSTANDING = 16384;

// ============================================================================
// 激励场景配置
// ============================================================================

enum class ScenarioType {
    A_MIXTRAL_L16,      // Mixtral 8x7B Layer 16
    B_DEEPSEEK_L30,     // DeepSeek-V3 Layer 30
    C_DEEPSEEK_PIPELINE // DeepSeek-V3 Pipeline 4 层
};

// 场景 A: Mixtral L16
constexpr int SCENARIO_A_CONCURRENT_TASKS = 8;
constexpr int SCENARIO_A_MEMBER_DIST[] = {140, 135, 130, 128, 125, 122, 120, 124};

// 场景 B: DeepSeek L30
constexpr int SCENARIO_B_CONCURRENT_TASKS = 256;
constexpr int SCENARIO_B_TOTAL_TOKENS = 9216;  // 1024 × 9

// 场景 C: DeepSeek Pipeline
constexpr int SCENARIO_C_LAYERS[] = {15, 30, 45, 60};
constexpr int SCENARIO_C_TASKS_PER_LAYER = 256;

// ============================================================================
// 仿真参数
// ============================================================================

constexpr double SIMULATION_TIME_NS = 1000000.0;  // 1 ms

#endif // SIM_CONFIG_H
