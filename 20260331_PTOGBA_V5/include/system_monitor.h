/**
 * @file system_monitor.h
 * @brief System Monitor 模块（三维指标）
 * @author Hinton (Silver Maine Coon)
 * @date 2026-03-31
 *
 * 三维指标：JCT / P99 / Max Queue Depth
 * 每一纳秒延迟都是犯罪。嗷唔。
 */

#ifndef SYSTEM_MONITOR_H
#define SYSTEM_MONITOR_H

#include <systemc>
#include <vector>
#include <algorithm>
#include "types.h"
#include "sim_config.h"

// ============================================================================
// System Monitor 模块
// ============================================================================
SC_MODULE(System_Monitor) {
public:
    // 输入端口：任务完成事件
    sc_in<BarrierTask> task_completed;
    
    // 输入端口：TX_Queue 深度采样
    sc_in<int> tx_queue_depth;
    
    SC_CTOR(System_Monitor);
    
    // 获取统计结果
    SimulationStats get_stats() const;
    
private:
    SimulationStats stats_;
    std::vector<double> task_latencies_;
    
    void record_completion();
    void sample_queue_depth();
};

// ============================================================================
// 实现
// ============================================================================
SC_CTOR(System_Monitor) : stats_({}) {
    SC_METHOD(record_completion);
    sensitive << task_completed;
    
    SC_METHOD(sample_queue_depth);
    sensitive << tx_queue_depth;
}

void System_Monitor::record_completion() {
    if (task_completed.event()) {
        BarrierTask task = task_completed.read();
        
        // 计算 E2E 时延
        double e2e_latency = task.scatter_complete_time - task.start_time;
        task_latencies_.push_back(e2e_latency);
        
        // 更新 JCT
        if (stats_.first_task_start_time == 0 || 
            task.start_time < stats_.first_task_start_time) {
            stats_.first_task_start_time = task.start_time;
        }
        
        if (task.scatter_complete_time > stats_.last_task_complete_time) {
            stats_.last_task_complete_time = task.scatter_complete_time;
        }
        
        // 更新路径统计
        switch (task.path) {
            case PathType::GBA: stats_.gba_count++; break;
            case PathType::MC:  stats_.mc_count++;  break;
            case PathType::P2P: stats_.p2p_count++; break;
        }
    }
}

void System_Monitor::sample_queue_depth() {
    int depth = tx_queue_depth.read();
    if (depth > stats_.max_tx_queue_depth) {
        stats_.max_tx_queue_depth = depth;
    }
}

SimulationStats System_Monitor::get_stats() const {
    SimulationStats result = stats_;
    
    // 计算 P99
    if (!task_latencies_.empty()) {
        // 复制到可变 vector 后排序
        std::vector<double> sorted_latencies = task_latencies_;
        std::sort(sorted_latencies.begin(), sorted_latencies.end());
        int p99_index = static_cast<int>(sorted_latencies.size() * 0.99);
        result.task_latencies = sorted_latencies;
    }
    
    return result;
}

#endif // SYSTEM_MONITOR_H
