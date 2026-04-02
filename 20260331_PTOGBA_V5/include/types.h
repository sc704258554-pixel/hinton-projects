/**
 * @file types.h
 * @brief V5 基础类型定义
 * @author Hinton (Silver Maine Coon)
 * @date 2026-03-31
 *
 * 禁止硬编码公式！模拟物理行为！
 * 每一纳秒延迟都是犯罪。嗷唔。
 */

#ifndef TYPES_H
#define TYPES_H

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <queue>
#include "sim_config.h"

// ============================================================================
// 包定义
// ============================================================================
struct Packet {
    int task_id;
    int member_count;
    int src_node;          // 源节点
    int dst_node;          // 目标节点（Master 或广播）
    bool is_completion;   // 是否完成信号
    double send_time;      // 发送时刻
    double arrive_time;   // 到达时刻
};

// ============================================================================
// 仿真统计
// ============================================================================
struct SimulationStats {
    // JCT（Job Completion Time）
    double first_task_start_time;
    double last_task_complete_time;
    
    // P99/P99.9 长尾时延
    std::vector<double> task_latencies;
    
    // Max Queue Depth
    int max_tx_queue_depth;
    
    // 路径分布
    int gba_count;
    int mc_count;
    int p2p_count;
    
    // GBA Slot 占用峰值
    int gba_slots_peak_usage;
    
    // MC Outstanding 占用峰值
    int mc_outstanding_peak_usage;
};

#endif // TYPES_H
