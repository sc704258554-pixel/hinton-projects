/**
 * @file sim_config.h
 * @brief V5 参数化配置（铲屎官修正版）
 * @author Hinton (Silver Maine Coon)
 * @date 2026-03-31
 *
 * 所有参数都必须参数化！禁止硬编码！
 * 每一纳秒延迟都是犯罪。嗷唔。
 */

#ifndef SIM_CONFIG_H
#define SIM_CONFIG_H

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <queue>

// ============================================================================
// 核心物理参数（铲屎官修正版）
// ============================================================================

// 第一段：物理链路时延（单向）
constexpr double T_LINK_OWD_NS = 10.0;  // 单程 2 米光纤物理极限

// 第二段：交换机流水线底噪
constexpr double T_PIPELINE_NS = 500.0;  // 交换机 Parser → Crossbar → TM

// 第三段：GBA 硬件处理（O(1)）
constexpr double T_GBA_PROC_NS = 10.0;  // SRAM 查表 + 状态机翻转

// 第三段：主机软件栈开销（PCIe + CPU 轮询）
constexpr double T_HOST_STACK_NS = 1000.0;  // 微秒级系统底噪

// 第三段：MC 单包复制惩罚（只复制指针！）
constexpr double T_MC_DUP_NS = 1.0;  // 1 GHz 芯片内部，1 个时钟周期

// 第三段：网卡串行化开销（PPS 瓶颈）
constexpr double T_SERIALIZE_NS = 20.0;  // 50 Mpps 连续发包极限

// ============================================================================
// 资源限制
// ============================================================================

constexpr int GBA_TOTAL_SLOTS = 512;
constexpr int MC_MAX_OUTSTANDING = 16384;

// ============================================================================
// 阈值配置
// ============================================================================

constexpr int GBA_THRESHOLD = 64;      // 大组阈值
constexpr int QUEUE_THRESHOLD = 100;    // TX_Queue 积压阈值

// ============================================================================
// 包路径类型
// ============================================================================
enum class PathType {
    GBA,    // 网内聚合（王者）
    MC,         // 异步多播（降级兜底）
    P2P         // 软件单点（地狱）
};

// ============================================================================
// 任务状态
// ============================================================================
enum class TaskState {
    PENDING,
    GATHERING,
    PROCESSING,
    SCATTERING,
    COMPLETED
};

// ============================================================================
// Barrier 任务定义
// ============================================================================
struct BarrierTask {
    int task_id;
    int member_count;      // N
    PathType path;           // 路径类型
    TaskState state;          // 任务状态
    double start_time;       // 发包时刻
    double gather_complete_time;  // Gather 完成时刻
    double scatter_complete_time; // Scatter 完成时刻
 // 实际占用的 Slot ID（GBA）或 Outstanding 数量（MC）|
    int slot_id;            // GBA 专用
    int outstanding_used;  // MC 专用
    
    double tail_latency;     // 首尾到达时间差
};

// ============================================================================
// 端到端时延公式（理论值，不硬编码！）
// ============================================================================

/**
 * @brief GBA 路径理论时延
 * E2E_GBA = 2 × T_link_owd + T_pipeline + T_gba_proc
 *             = 20 + 500 + 10 = 530 ns（无 tail）
 */
 
/**
 * @brief MC 路径理论时延
 * E2E_MC = 4 × T_link_owd + 2 × T_pipeline + T_host_stack + N × T_mc_dup
 *             = 40 + 1000 + 1000 + N × 1
 *             = 2040 + N ns
 */

/**
 * @brief P2P 路径理论时延
 * E2E_P2P = 4 × T_link_owd + 2 × T_pipeline + T_host_stack + (N-1) × T_serialize
 *             = 40 + 1000 + 1000 + (N-1) × 20
 *             = 2040 + 20 × (N-1) ns
 */

#endif // SIM_CONFIG_H
