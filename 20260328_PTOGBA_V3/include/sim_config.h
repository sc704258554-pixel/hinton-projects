/**
 * @file sim_config.h
 * @brief PTO-GBA ESL 仿真全局配置 V3.0
 * @author Hinton (Silver Maine Coon)
 * @date 2026-03-28
 *
 * V3.0 改进：真实 MoE 激励场景 + 端到端时延对比
 * 每一纳秒延迟都是犯罪。嗷唔。
 */

#ifndef SIM_CONFIG_H
#define SIM_CONFIG_H

#include <cstdint>

// ============================================================================
// 拓扑参数
// ============================================================================
constexpr uint16_t NUM_SWITCHES = 16;
constexpr uint16_t GBA_SLOTS_PER_SWITCH = 32;
constexpr uint16_t MC_OUTSTANDING_PER_SWITCH = 1024;

constexpr uint32_t TOTAL_GBA_SLOTS = NUM_SWITCHES * GBA_SLOTS_PER_SWITCH;       // 512
constexpr uint32_t TOTAL_MC_OUTSTANDING = NUM_SWITCHES * MC_OUTSTANDING_PER_SWITCH; // 16384

// ============================================================================
// 动态时延 Base 值
// ============================================================================
constexpr double GBA_BASE_DELAY_NS = 50.0;
constexpr double MC_BASE_DELAY_NS = 200.0;
constexpr double HOST_BASE_DELAY_NS = 5000.0;
constexpr double SRAM_LOOKUP_NS = 5.0;
constexpr double PCIE_OVERHEAD_NS = 1000.0;
constexpr double NETWORK_RTT_NS = 20.0;
constexpr double MICROCODE_DELAY_NS = 100.0;

// ============================================================================
// Arbiter 阈值
// ============================================================================
constexpr uint16_t GBA_THRESHOLD_N = 64;
constexpr uint16_t MC_THRESHOLD_N = 16;
constexpr double GBA_RESERVE_RATIO = 0.2;
constexpr double TIMEOUT_MS = 10.0;

// ============================================================================
// 并发持有资源参数
// ============================================================================
constexpr double RESOURCE_HOLDING_TIME_US = 100.0;
constexpr double RESOURCE_HOLDING_TIME_NS = RESOURCE_HOLDING_TIME_US * 1000.0;

// ============================================================================
// 惩罚系数
// ============================================================================
constexpr double CONGESTION_PENALTY_FACTOR = 1.5;
constexpr double BACKPRESSURE_THRESHOLD = 0.8;
constexpr double SOFTWARE_JITTER_SIGMA = 0.5;
constexpr double SOFTWARE_JITTER_MU = 0.0;

// ============================================================================
// V3: 真实 MoE 模型参数
// ============================================================================
constexpr int MIXTRAL_LAYERS = 32;
constexpr int MIXTRAL_EXPERTS = 8;
constexpr int MIXTRAL_TOP_K = 2;

constexpr int DEEPSEEK_LAYERS = 61;
constexpr int DEEPSEEK_EXPERTS = 256;
constexpr int DEEPSEEK_TOP_K = 9;

// ============================================================================
// V3 激励场景
// ============================================================================
constexpr int SCENARIO_A_NUM_TASKS = 8;
constexpr int SCENARIO_A_N_MIN = 120;
constexpr int SCENARIO_A_N_MAX = 140;

constexpr int SCENARIO_B_NUM_TASKS = 256;
constexpr int SCENARIO_B_N_MIN = 10;
constexpr int SCENARIO_B_N_MAX = 100;

constexpr int SCENARIO_C_NUM_LAYERS = 4;
constexpr int SCENARIO_C_NUM_TASKS = SCENARIO_C_NUM_LAYERS * DEEPSEEK_EXPERTS;

// ============================================================================
// V3: 算法对比模式
// ============================================================================
enum class ComparisonMode : uint8_t {
    V3_ALGORITHM = 0,
    V2_ALGORITHM = 1,
    ALL_HOST = 2
};

constexpr ComparisonMode DEFAULT_COMPARISON_MODE = ComparisonMode::V3_ALGORITHM;

// ============================================================================
// 仿真参数
// ============================================================================
constexpr double SIM_TIME_NS = 10000000.0;
constexpr double SIM_TIME_RESOLUTION_NS = 1.0;
constexpr uint32_t RANDOM_SEED = 42;

constexpr const char* STATS_OUTPUT_PATH = "logs/simulation_results_v3.txt";

#endif // SIM_CONFIG_H
