/**
 * @file types.h
 * @brief V5.1 基础类型定义
 * @author Hinton (Silver Maine Coon)
 * @date 2026-03-31
 *
 * 禁止硬编码公式！模拟物理行为！
 * 每一纳秒延迟都是犯罪。嗷唔。
 */

#ifndef TYPES_H
#define TYPES_H

#include <systemc>
#include <vector>
#include <queue>
#include <cstdint>
#include "sim_config.h"

// ============================================================================
// 包定义
// ============================================================================
struct Packet {
    int task_id;
    int member_count;
    int src_node;
    int dst_node;
    bool is_completion;
    double send_time;
    double arrive_time;
    
    // SystemC 信号支持
    bool operator==(const Packet& rhs) const {
        return task_id == rhs.task_id && 
               member_count == rhs.member_count &&
               src_node == rhs.src_node &&
               dst_node == rhs.dst_node;
    }
    
    friend std::ostream& operator<<(std::ostream& os, const Packet& pkt) {
        os << "Packet{task=" << pkt.task_id 
           << ", N=" << pkt.member_count << "}";
        return os;
    }
};

// ============================================================================
// Barrier 任务定义
// ============================================================================
struct BarrierTask {
    int task_id;
    int member_count;      // N
    PathType path;         // 路径类型
    TaskState state;       // 任务状态
    double start_time;     // 发包时刻
    double gather_complete_time;  // Gather 完成时刻
    double scatter_complete_time; // Scatter 完成时刻
    int slot_id;           // GBA 专用
    int outstanding_used;  // MC 专用
    double tail_latency;   // 首尾到达时间差
    int priority;          // 优先级（用于优先队列）
    
    // 优先队列比较器（大组优先）
    bool operator<(const BarrierTask& other) const {
        // 优先级高的（member_count 大的）排在前面
        return member_count < other.member_count;
    }
};

// ============================================================================
// 等待队列任务包装
// ============================================================================
struct WaitingTask {
    int task_id;
    int member_count;
    double arrival_time;
    
    // 优先队列比较器（大组优先）
    bool operator<(const WaitingTask& other) const {
        // member_count 大的排在前面（优先级高）
        if (member_count != other.member_count) {
            return member_count < other.member_count;
        }
        // member_count 相同时，先到的优先
        return arrival_time > other.arrival_time;
    }
};

// ============================================================================
// 仿真统计
// ============================================================================
struct SimulationStats {
    double first_task_start = 0.0;
    double last_task_complete = 0.0;
    std::vector<double> task_latencies;
    int max_tx_queue_depth = 0;
    int gba_count = 0;
    int mc_count = 0;
    int p2p_count = 0;
    int gba_slots_peak = 0;
    int mc_outstanding_peak = 0;
};

#endif // TYPES_H
