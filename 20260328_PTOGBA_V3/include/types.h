/**
 * @file types.h
 * @brief PTO-GBA ESL 仿真类型定义
 * @author Hinton (Silver Maine Coon)
 * @date 2026-03-27
 */

#ifndef TYPES_H
#define TYPES_H

#include <cstdint>
#include <string>
#include <iostream>
#include <systemc>
#include "sim_config.h"  // 需要 GBA_THRESHOLD_N, MC_THRESHOLD_N

// ============================================================================
// 路径枚举 (Path Type)
// ============================================================================
enum class PathType : uint8_t {
    GBA = 0,    // 硬件极速通道
    MC  = 1,    // 异步多播兜底
    HOST = 2    // 软件兜底 (地狱通道)
};

// 路径类型转字符串
inline std::string path_to_string(PathType path) {
    switch (path) {
        case PathType::GBA:  return "GBA";
        case PathType::MC:   return "MC";
        case PathType::HOST: return "HOST";
        default: return "UNKNOWN";
    }
}

// ============================================================================
// Barrier 请求结构体 (Barrier Request)
// ============================================================================
struct BarrierRequest {
    uint16_t task_id;      // 任务唯一标识
    uint16_t group_size;   // N: 逻辑成员数
    uint16_t priority;     // 优先级 (0=低, 1=高)
    uint32_t timeout_ns;   // 超时时间
    uint8_t  path_hint;    // 0=auto, 1=GBA, 2=MC, 3=HOST
    uint8_t  wave_id;      // V3: 波次标记（第几波）
    sc_core::sc_time arrival_time;  // 请求到达时间

    // 默认构造
    BarrierRequest()
        : task_id(0), group_size(0), priority(0),
          timeout_ns(10000000), path_hint(0), wave_id(0),
          arrival_time(sc_core::SC_ZERO_TIME) {}

    // 带参数构造
    BarrierRequest(uint16_t tid, uint16_t n, uint16_t prio,
                   uint32_t timeout = 10000000, uint8_t hint = 0, uint8_t wave = 0)
        : task_id(tid), group_size(n), priority(prio),
          timeout_ns(timeout), path_hint(hint), wave_id(wave),
          arrival_time(sc_core::SC_ZERO_TIME) {}
};

// ============================================================================
// Barrier 响应结构体 (Barrier Response)
// ============================================================================
struct BarrierResponse {
    uint16_t task_id;      // 任务唯一标识
    PathType  path_used;   // 实际使用的路径
    double    latency_ns;  // E2E 时延 (ns)
    bool      success;     // 是否成功
    bool      timeout;     // 是否超时
    uint8_t   wave_id;     // V3: 波次标记
    sc_core::sc_time arrival_time;     // V3: 到达时间
    sc_core::sc_time completion_time;  // 完成时间

    // 默认构造
    BarrierResponse()
        : task_id(0), path_used(PathType::HOST),
          latency_ns(0.0), success(false), timeout(false), wave_id(0),
          arrival_time(sc_core::SC_ZERO_TIME),
          completion_time(sc_core::SC_ZERO_TIME) {}
};

// ============================================================================
// 资源状态结构体 (Resource Status)
// ============================================================================
struct ResourceStatus {
    uint32_t gba_slots_used;        // 已用 GBA slots
    uint32_t mc_outstanding_used;   // 已用 MC outstanding
    double   gba_utilization;       // GBA 利用率 [0, 1]
    double   mc_utilization;        // MC 利用率 [0, 1]
    double   congestion_factor;     // 拥塞因子 [0, 1]
    double   queue_depth;           // 队列深度

    // 默认构造
    ResourceStatus()
        : gba_slots_used(0), mc_outstanding_used(0),
          gba_utilization(0.0), mc_utilization(0.0),
          congestion_factor(0.0), queue_depth(0.0) {}

    // 检查是否有可用 GBA slot
    bool has_gba_slot(uint32_t total_slots) const {
        return gba_slots_used < total_slots;
    }

    // 检查是否有足够 MC outstanding (需要 N 个)
    bool has_mc_capacity(uint16_t n, uint32_t total_outstanding) const {
        return (mc_outstanding_used + n) <= total_outstanding;
    }

    // 更新利用率
    void update_utilization(uint32_t total_gba, uint32_t total_mc) {
        gba_utilization = static_cast<double>(gba_slots_used) / total_gba;
        mc_utilization = static_cast<double>(mc_outstanding_used) / total_mc;
        congestion_factor = std::max(gba_utilization, mc_utilization);
    }
};

// ============================================================================
// 分组大小类别 (Group Size Category)
// ============================================================================
enum class GroupCategory : uint8_t {
    SMALL,    // N < 16
    MEDIUM,   // 16 <= N < 64
    LARGE     // N >= 64
};

// 根据组大小判断类别
inline GroupCategory get_category(uint16_t n) {
    if (n >= GBA_THRESHOLD_N) return GroupCategory::LARGE;
    if (n < MC_THRESHOLD_N) return GroupCategory::SMALL;
    return GroupCategory::MEDIUM;
}

// 类别转字符串
inline std::string category_to_string(GroupCategory cat) {
    switch (cat) {
        case GroupCategory::SMALL:  return "SMALL";
        case GroupCategory::MEDIUM: return "MEDIUM";
        case GroupCategory::LARGE:  return "LARGE";
        default: return "UNKNOWN";
    }
}

// ============================================================================
// operator<< 重载 (SystemC sc_fifo 需要)
// ============================================================================
inline std::ostream& operator<<(std::ostream& os, const BarrierRequest& req) {
    os << "BarrierRequest{task_id=" << req.task_id
       << ", group_size=" << req.group_size
       << ", priority=" << req.priority << "}";
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const BarrierResponse& resp) {
    os << "BarrierResponse{task_id=" << resp.task_id
       << ", path=" << path_to_string(resp.path_used)
       << ", latency=" << resp.latency_ns << "ns"
       << ", success=" << (resp.success ? "yes" : "no") << "}";
    return os;
}

// ============================================================================
// 场景枚举 (Scenario Type)
// ============================================================================
enum class ScenarioType : uint8_t {
    FRAGMENTED_SMALL,   // 海量碎片小组
    MIXED_CONCURRENT,   // 混合规模并发
    TAIL_LATENCY,       // 长尾掉队者
    BURST_SPIKE,        // 突发洪峰
    // V3 新增
    MIXTRAL_LAYER_16,
    DEEPSEEK_LAYER_30,
    DEEPSEEK_MULTI_LAYER
};

// V3 场景枚举
enum class ScenarioTypeV3 : uint8_t {
    MIXTRAL_LAYER_16 = 0,
    DEEPSEEK_LAYER_30 = 1,
    DEEPSEEK_MULTI_LAYER = 2
};

#endif // TYPES_H
