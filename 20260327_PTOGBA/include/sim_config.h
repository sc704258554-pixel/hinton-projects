/**
 * @file sim_config.h
 * @brief PTO-GBA ESL 仿真全局配置
 * @author Hinton (Silver Maine Coon)
 * @date 2026-03-27
 *
 * 所有参数统一收拢，方便后续拿流片数据对齐。
 * 每一纳秒延迟都是犯罪。嗷唔。
 */

#ifndef SIM_CONFIG_H
#define SIM_CONFIG_H

#include <cstdint>

// ============================================================================
// 拓扑参数 (Topology)
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
// GBA: 纯硬件汇聚，极致快速
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

// ============================================================================
// 场景开关 (Scenario Switches)
// ============================================================================
// 场景 1: 海量碎片小组
constexpr bool SCENARIO_FRAGMENTED_SMALL = true;

// 场景 2: 混合规模并发
constexpr bool SCENARIO_MIXED_CONCURRENT = true;

// 场景 3: 长尾掉队者
constexpr bool SCENARIO_TAIL_LATENCY = true;

// 场景 4: 突发洪峰
constexpr bool SCENARIO_BURST_SPIKE = true;

// ============================================================================
// 仿真参数 (Simulation Parameters)
// ============================================================================
// 仿真时间 (ns)
constexpr double SIM_TIME_NS = 1000000.0;  // 1ms

// 仿真时间分辨率
constexpr double SIM_TIME_RESOLUTION_NS = 1.0;

// 随机种子
constexpr uint32_t RANDOM_SEED = 42;

// ============================================================================
// 统计输出路径
// ============================================================================
constexpr const char* STATS_OUTPUT_PATH = "logs/simulation_results.txt";

#endif // SIM_CONFIG_H
