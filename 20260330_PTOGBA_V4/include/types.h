/**
 * @file types.h
 * @brief 基础类型定义
 * @author Hinton (Silver Maine Coon)
 * @date 2026-03-31
 *
 * 每一纳秒延迟都是犯罪。嗷唔。
 */

#ifndef TYPES_H
#define TYPES_H

#include <cstdint>
#include <string>
#include <systemc>

// ============================================================================
// 路径类型枚举
// ============================================================================
enum class PathType {
    GBA,    // 硬件极速通道 (Group Barrier Acceleration)
    MC,     // 异步多播兜底通道 (Multicast)
    HOST    // 软件兜底 (Host CPU)
};

// ============================================================================
// 资源状态
// ============================================================================
struct ResourceStatus {
    uint32_t gba_slots_used;        // 已用 GBA slots
    uint32_t mc_outstanding_used;    // 已用 MC outstanding
    double congestion_factor;        // 拥塞因子 [0, 1]
    double queue_depth;              // 队列深度

    ResourceStatus()
        : gba_slots_used(0)
        , mc_outstanding_used(0)
        , congestion_factor(0.0)
        , queue_depth(0.0)
    {}
};

// ============================================================================
// 同步请求
// ============================================================================
struct BarrierRequest {
    uint64_t task_id;               // 任务 ID
    uint16_t group_size;             // 组大小 N (member 数量)
    uint16_t priority;               // 优先级 (0=普通, >0=高优先级)
    uint32_t wave_id;                // V3: 波次标记
    sc_core::sc_time arrival_time;   // V3: 到达时间

    BarrierRequest()
        : task_id(0)
        , group_size(0)
        , priority(0)
        , wave_id(0)
        , arrival_time(sc_core::SC_ZERO_TIME)
    {}
};

// ============================================================================
// 同步响应
// ============================================================================
struct BarrierResponse {
    uint64_t task_id;               // 任务 ID
    PathType path_used;             // 使用的路径
    double latency_ns;               // 端到端时延 (ns)
    bool success;                    // 是否成功
    bool timeout;                    // 是否超时
    uint32_t wave_id;                // V3: 波次标记
    sc_core::sc_time arrival_time;   // V3: 到达时间
    sc_core::sc_time completion_time; // V3: 完成时间

    BarrierResponse()
        : task_id(0)
        , path_used(PathType::HOST)
        , latency_ns(0.0)
        , success(false)
        , timeout(false)
        , wave_id(0)
        , arrival_time(sc_core::SC_ZERO_TIME)
        , completion_time(sc_core::SC_ZERO_TIME)
    {}
};

// ============================================================================
// V4: 时延分解（用于统计）
// ============================================================================
struct LatencyBreakdown {
    double t_link_ns;       // 第一段：物理链路 RTT
    double t_pipeline_ns;   // 第二段：流水线底噪
    double t_process_ns;    // 第三段：核心处理与占坑
    double total_ns;        // 总时延

    LatencyBreakdown()
        : t_link_ns(0.0)
        , t_pipeline_ns(0.0)
        , t_process_ns(0.0)
        , total_ns(0.0)
    {}
};

#endif // TYPES_H
