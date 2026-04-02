/**
 * @file arbiter.h
 * @brief Arbiter 模块（感知端侧压力）
 * @author Hinton (Silver Maine Coon)
 * @date 2026-03-31
 *
 * 降级逻辑：GBA > MC > P2P
 * 感知 TX_Queue_Depth，端侧压力大时强制走 GBA
 * 每一纳秒延迟都是犯罪。嗷唔。
 */

#ifndef ARBITER_H
#define ARBITER_H

#include <systemc>
#include "types.h"
#include "sim_config.h"

// ============================================================================
// Arbiter 模块
// ============================================================================
SC_MODULE(Arbiter) {
public:
    // 输入端口：来自端侧的任务请求
    sc_in<BarrierTask> task_request;
    
    // 输入端口：端侧 TX_Queue 深度（压力感知！）
    sc_in<int> tx_queue_depth;
    
    // 输入端口：GBA 和 MC 资源占用
    sc_in<int> gba_slots_used;
    sc_in<int> mc_outstanding_used;
    sc_in<bool> gba_slot_full;
    sc_in<bool> mc_outstanding_full;
    
    // 输出端口：选择的路径
    sc_out<PathType> selected_path;
    
    SC_CTOR(Arbiter);
    
private:
    void arbitrate();
};

// ============================================================================
// 实现
// ============================================================================
SC_CTOR(Arbiter) {
    SC_METHOD(arbitrate);
    sensitive << task_request << tx_queue_depth 
                << gba_slots_used << mc_outstanding_used
 
                << gba_slot_full << mc_outstanding_full;
 // 初始化队列阈值
 // 降级逻辑：GBA > MC > P2P
    // 1. 感知端侧压力（铲屎官修正！）
    if (tx_queue_depth.read() > QUEUE_THRESHOLD) {
        // 端侧压力大，强制走 GBA
        selected_path.write(PathType::GBA);
        return;
    }
    
    // 2. GBA 路径（王者）
    BarrierTask task = task_request.read();
    
    // 大组（N >= 64） 优先 GBA
    if (task.member_count >= GBA_THRESHOLD && !gba_slot_full.read()) {
        selected_path.write(PathType::GBA);
        return;
    }
    
    // 3. MC 路径（降级兜底)
    int mc_needed = task.member_count;
    if (!mc_outstanding_full.read() && 
        mc_outstanding_used.read() + mc_needed <= MC_MAX_OUTSTANDING) {
        selected_path.write(PathType::MC);
        return;
    }
    
    // 4. P2P 路径（地狱）
    selected_path.write(PathType::P2P);
}

#endif // ARBITER_H
