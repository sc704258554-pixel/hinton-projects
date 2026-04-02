/**
 * @file gba_engine.h
 * @brief GBA 引擎模块（占坑卡死！）
 * @author Hinton (Silver Maine Coon)
 * @date 2026-03-31
 *
 * 资源锁定：首包到达 → 锁定 Slot
 *              尾包到达 → 释放 Slot
 * 每一纳秒延迟都是犯罪。嗷唔。
 */

#ifndef GBA_ENGINE_H
#define GBA_ENGINE_H

#include <systemc>
#include <map>
#include <vector>
#include "types.h"
#include "sim_config.h"

// ============================================================================
// GBA 引擎模块
// ============================================================================
SC_MODULE(GBA_Engine) {
public:
    // 输入端口：接收来自 Switch_Port 的包
    sc_in<Packet> packet_in;
    
    // 输出端口：发送完成信号到 Switch_Port
    sc_out<Packet> packet_out;
    
    // 监控端口：当前 Slot 占用情况
    sc_out<int> slots_used;
    sc_out<bool> slot_full;  // 512 个 Slot 全部占用
    
    SC_CTOR(GBA_Engine);
    
private:
    bool slot_busy_[GBA_TOTAL_SLOTS];        // 512 个 Slot
    std::map<int, int> task_to_slot_;        // task_id → slot_id
        std::map<int, int> task_member_arrived_; // task_id → 已到达成员数
    std::map<int, std::vector<double>> task_arrival_times_; // task_id → 各成员到达时刻
    
    std::map<int, double> task_first_arrival_;  // task_id → 首包到达时刻
    std::map<int, double> task_last_arrival_;  // task_id → 尾包到达时刻
    
    int slots_used_;
    
    void process_packet();
    int allocate_slot(int task_id);
    void release_slot(int task_id);
};

// ============================================================================
// 实现
// ============================================================================
SC_CTOR(GBA_Engine) : slots_used_(0) {
    // 初始化所有 Slot 为空闲
    for (int i = 0; i < GBA_TOTAL_SLOTS; i++) {
        slot_busy_[i] = false;
    }
    
    SC_METHOD(process_packet);
    sensitive << packet_in;
}

int GBA_Engine::allocate_slot(int task_id) {
    // 寻找空闲 Slot
    for (int i = 0; i < GBA_TOTAL_SLOTS; i++) {
        if (!slot_busy_[i]) {
            slot_busy_[i] = true;  // 首包到达 → 锁定 Slot
            task_to_slot_[task_id] = i;
            slots_used_++;
            return i;
        }
    }
    return -1;  // 没有空闲 Slot
}

void GBA_Engine::release_slot(int task_id) {
    auto it = task_to_slot_.find(task_id);
    if (it != task_to_slot_.end()) {
        int slot_id = it->second;
        slot_busy_[slot_id] = false;  // 尾包到达 → 释放 Slot
        task_to_slot_.erase(it);
        slots_used_--;
    }
}

void GBA_Engine::process_packet() {
    if (packet_in.event()) {
        Packet pkt = packet_in.read();
        
        // 模拟 GBA 硬件处理延迟（10 ns）
        wait(T_GBA_PROC_NS, SC_NS);
        
        // 检查任务是否已分配 Slot
        if (task_to_slot_.find(pkt.task_id) == task_to_slot_.end()) {
            // 第一个包到达 → 分配 Slot
            int slot_id = allocate_slot(pkt.task_id);
            if (slot_id == -1) {
                // Slot 满了，降级到 MC（实际由 Arbiter 处理）
                return;
            }
            
            // 记录首包到达时刻
            task_first_arrival_[pkt.task_id] = sc_time_stamp().to_seconds() * 1e9;
            task_member_arrived_[pkt.task_id] = 1;
            task_arrival_times_[pkt.task_id].push(sc_time_stamp().to_seconds() * 1e9);
        } else {
            // 后续包到达
            task_member_arrived_[pkt.task_id]++;
            task_arrival_times_[pkt.task_id].push(sc_time_stamp().to_seconds() * 1e9);
            
            // 检查是否所有成员都到了
            if (task_member_arrived_[pkt.task_id] == pkt.member_count) {
                // 尾包到达 → 记录时刻
                task_last_arrival_[pkt.task_id] = sc_time_stamp().to_seconds() * 1e9;
                
                // 计算首尾到达时间差（tail latency）
                double tail_latency = task_last_arrival_[pkt.task_id] - task_first_arrival_[pkt.task_id];
                
                // 释放 Slot
                release_slot(pkt.task_id);
                
                // 发送完成信号（多播）
                Packet completion_pkt;
                completion_pkt.task_id = pkt.task_id;
                completion_pkt.member_count = pkt.member_count;
                completion_pkt.is_completion = true;
                completion_pkt.send_time = sc_time_stamp().to_seconds() * 1e9;
                
                packet_out.write(completion_pkt);
                
                // 清理任务数据
                task_member_arrived_.erase(pkt.task_id);
                task_arrival_times_.erase(pkt.task_id);
                task_first_arrival_.erase(pkt.task_id);
                task_last_arrival_.erase(pkt.task_id);
            }
        }
        
        // 更新监控端口
        slots_used.write(slots_used_);
        slot_full.write(slots_used_ >= GBA_TOTAL_SLOTS);
    }
}

#endif // GBA_ENGINE_H
