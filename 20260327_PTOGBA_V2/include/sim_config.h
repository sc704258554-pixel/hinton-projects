/**
 * @file sim_config_v2.h
 * @brief PTO-GBA ESL 仿真全局配置 V2.0
 * @author Hinton (Silver Maine Coon)
 * @date 2026-03-27
 *
 * V2.0 改进：极限激励场景 + Straggler 处理 + 公平队列
 * 每一纳秒延迟都是犯罪。嗷唔。
 */

#ifndef SIM_CONFIG_V2_H
#define SIM_CONFIG_V2_H

#include <cstdint>

// ============================================================================
// 拓扑参数 (Topology) - 与 V1.0 相同
// ============================================================================
constexpr uint16_t NUM_SWITCHES = 16;
constexpr uint16_t GBA_SLOTS_PER_SWITCH = 32;
constexpr uint16_t MC_OUTSTANDING_PER_SWITCH = 1024;

// 全网总容量
constexpr uint32_t TOTAL_GBA_SLOTS = NUM_SWITCHES * GBA_SLOTS_PER_SWITCH;       // 512
constexpr uint32_t TOTAL_MC_OUTSTANDING = NUM_SWITCHES * MC_OUTSTANDING_PER_SWITCH; // 16384

// 物理线缆长度
constexpr double CABLE_LENGTH_M = 2.0;  // 2 米

// ============================================================================
// 动态时延 Base 值 (Dynamic Latency Base Values, ns)
// ============================================================================
// GBA: 纯硬件汇聚，与 N 无关（非对称消耗特性！）
constexpr double GBA_BASE_DELAY_NS = 50.0;

// MC: 微码复制引擎
constexpr double MC_BASE_DELAY_NS = 200.0;

// HOST: 软件兜底 (地狱通道)
constexpr double HOST_BASE_DELAY_NS = 5000.0;

// SRAM 查表
constexpr double SRAM_LOOKUP_NS = 5.0;

// PCIe 开销
constexpr double PCIE_OVERHEAD_NS = 1000.0;

// 网络传输 (2m 线缆 RTT, 光速限制)
constexpr double NETWORK_RTT_NS = 20.0;

// 微码处理
constexpr double MICROCODE_DELAY_NS = 100.0;

// ============================================================================
// V2.2 新增: 并发持有资源参数
// ============================================================================
// 资源持有时间 (us 级) - 模拟真实同步操作占用资源的时间
constexpr double RESOURCE_HOLDING_TIME_US = 100.0;    // 100 us
constexpr double RESOURCE_HOLDING_TIME_NS = RESOURCE_HOLDING_TIME_US * 1000.0;

// 批量突发参数 - 更激进的突发
constexpr uint32_t BURST_BATCH_SIZE = 500;            // 每批突发 500 个请求
constexpr double BURST_ARRIVAL_INTERVAL_NS = 10.0;    // 批次间隔 10 ns（极短）

// Straggler 参数（通用）
constexpr double STRAGGLER_DELAY_MS = 5.0;            // Straggler 额外延迟

// ============================================================================
// V2.0 新增: 公平队列参数
// ============================================================================
constexpr uint8_t NUM_FAIRNESS_QUEUES = 4;      // 按 N 分类: SMALL/MEDIUM/LARGE/XLARGE
constexpr uint32_t STARVATION_THRESHOLD_MS = 100;  // 超过此等待时间视为饿死风险
constexpr double AGING_FACTOR = 0.1;             // 老化系数：每等待 1ms 优先级 +0.1

// ============================================================================
// 惩罚系数 (Penalty Factors)
// ============================================================================
// 拥塞惩罚因子
constexpr double CONGESTION_PENALTY_FACTOR = 1.5;

// 背压阈值 (队列深度超过此值触发惩罚)
constexpr double BACKPRESSURE_THRESHOLD = 0.8;

// 软件抖动 sigma (对数正态分布)
constexpr double SOFTWARE_JITTER_SIGMA = 0.5;

// 软件抖动 mu (对数正态分布)
constexpr double SOFTWARE_JITTER_MU = 0.0;

// V2.0 新增: 突发惩罚乘数
constexpr double BURST_PENALTY_MULTIPLIER = 2.0;

// ============================================================================
// Arbiter 阈值 (Arbiter Thresholds)
// ============================================================================
// 大组阈值: N >= 此值强制 GBA
constexpr uint16_t GBA_THRESHOLD_N = 64;

// 小组阈值: N < 此值默认 MC
constexpr uint16_t MC_THRESHOLD_N = 16;

// GBA 资源预留比例 (为大组预留)
constexpr double GBA_RESERVE_RATIO = 0.2;

// 超时降级时间 (ms)
constexpr double TIMEOUT_MS = 10.0;

// V2.0 新增: 负载均衡阈值
constexpr double GBA_LOAD_THRESHOLD = 0.9;   // GBA 利用率超过 90% 时优先 MC
constexpr double MC_LOAD_THRESHOLD = 0.9;    // MC 利用率超过 90% 时优先 HOST

// ============================================================================
// V2.0 极限激励场景参数
// ============================================================================

// === 场景 A: MoE 训练常态 (中等压力) ===
constexpr uint32_t SCENARIO_A_TOTAL_TASKS = 2000;
constexpr double SCENARIO_A_ARRIVAL_RATE = 100.0;  // 任务/时间单位
constexpr double SCENARIO_A_STRAGGLER_RATIO = 0.10;  // 10% 掉队者
constexpr double SCENARIO_A_STRAGGLER_DELAY_MS = 5.0;

// === 场景 B: MoE 训练洪峰 (极限压力) ===
constexpr uint32_t SCENARIO_B_TOTAL_TASKS = 5000;
constexpr double SCENARIO_B_ARRIVAL_RATE = 500.0;  // 突发高到达率
constexpr double SCENARIO_B_STRAGGLER_RATIO = 0.20;  // 20% 掉队者
constexpr double SCENARIO_B_BURST_PROBABILITY = 0.3;  // 30% 概率突发

// === 场景 C: 恶意压测 (压力测试) ===
constexpr uint32_t SCENARIO_C_TOTAL_TASKS = 10000;
constexpr double SCENARIO_C_ARRIVAL_RATE = 1000.0;
constexpr double SCENARIO_C_STRAGGLER_RATIO = 0.30;  // 30% 掉队者

// ============================================================================
// V2.0 场景开关 (Scenario Switches)
// ============================================================================
enum class ScenarioTypeV2 {
    SCENARIO_A_MOE_NORMAL,     // MoE 常态
    SCENARIO_B_MOE_BURST,      // MoE 洪峰
    SCENARIO_C_MALICIOUS       // 恶意压测
};

// ============================================================================
// 仿真参数 (Simulation Parameters)
// ============================================================================
// 仿真时间 (ns)
constexpr double SIM_TIME_NS = 5000000.0;  // 5ms (更长以容纳更多任务)

// 仿真时间分辨率
constexpr double SIM_TIME_RESOLUTION_NS = 1.0;

// 随机种子
constexpr uint32_t RANDOM_SEED = 42;

// ============================================================================
// 统计输出路径
// ============================================================================
constexpr const char* STATS_OUTPUT_PATH_V2 = "logs/simulation_results_v2.txt";

#endif // SIM_CONFIG_V2_H
