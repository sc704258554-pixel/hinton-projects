/**
 * @file mc_engine.h
 * @brief MC 引擎模块
 * @author Hinton (Silver Maine Coon)
 * @date 2026-03-31
 *
 * 模拟物理行为：线性复制惩罚（只复制指针！）
 * 每一纳秒延迟都是犯罪。嗷唔。
 */

#ifndef MC_ENGINE_H
#define MC_ENGINE_H

#include <systemc>
#include <map>
#include "types.h"
#include "sim_config.h"

// ============================================================================
// MC 引擎模块
// ============================================================================
SC_MODULE(MC_Engine) {
public:
    // 输入端口：接收来自 Switch_Port 的多播请求
    sc_in<Packet> packet_in;
    
    // 输出端口：发送多播响应到 Switch_Port
    sc_out<Packet> packet_out;
    
    // 监控端口：当前 Outstanding 占用
    sc_out<int> outstanding_used;
    sc_out<bool> outstanding_full;
    
    SC_CTOR(MC_Engine);
    
private:
    int outstanding_used_;
    
    void process_packet();
};

// ============================================================================
// 实现
// ============================================================================
SC_CTOR(MC_Engine) : outstanding_used_(0) {
    SC_METHOD(process_packet);
    sensitive << packet_in;
}

void MC_Engine::process_packet() {
    if (packet_in.event()) {
        Packet pkt = packet_in.read();
        
        // 检查 Outstanding 资源是否足够
        if (outstanding_used_ + pkt.member_count > MC_MAX_OUTSTANDING) {
            // 资源不足，降级到 P2P（实际由 Arbiter 处理）
            return;
        }
        
        // 分配 Outstanding
        outstanding_used_ += pkt.member_count;
        
        // 模拟 N 次指针复制（1 ns × N）
        wait(pkt.member_count * T_MC_DUP_NS, SC_NS);
        
        // 发送多播完成信号
        Packet completion_pkt;
        completion_pkt.task_id = pkt.task_id;
        completion_pkt.member_count = pkt.member_count;
        completion_pkt.is_completion = true;
        completion_pkt.send_time = sc_time_stamp().to_seconds() * 1e9;
        
        packet_out.write(completion_pkt);
        
        // 释放 Outstanding
        outstanding_used_ -= pkt.member_count;
        
        // 更新监控端口
        outstanding_used.write(outstanding_used_);
        outstanding_full.write(outstanding_used_ >= MC_MAX_OUTSTANDING);
    }
}

#endif // MC_ENGINE_H
