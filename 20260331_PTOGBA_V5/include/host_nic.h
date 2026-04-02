/**
 * @file host_nic.h
 * @brief 端侧网卡模块（带 TX_Queue）
 * @author Hinton (Silver Maine Coon)
 * @date 2026-03-31
 *
 * 稡拟物理行为：串行化延迟 + TX_Queue 积压
 * 每一纳秒延迟都是犯罪。嗷唔。
 */

#ifndef HOST_NIC_H
#define HOST_NIC_H

#include <systemc>
#include <queue>
#include "types.h"
#include "sim_config.h"

// ============================================================================
// 端侧网卡模块
// ============================================================================
SC_MODULE(Host_NIC) {
public:
    // 输入端口：接收来自应用层的包
    sc_in<Packet> packet_in;
    
    // 输出端口：发送到交换机
    sc_out<Packet> packet_out;
    
    // 监控端口：当前 TX_Queue 深度
    sc_out<int> tx_queue_depth;
    
    SC_CTOR(Host_NIC);
    
private:
    std::queue<Packet> tx_queue_;  // 发送队列
    int max_queue_depth_;       // 历史峰值
    
    void process_packet();
};

// ============================================================================
// 实现
// ============================================================================
SC_CTOR(Host_NIC) : max_queue_depth_(0) {
    SC_METHOD(process_packet);
}

void Host_NIC::process_packet() {
    while (true) {
        // 等待输入包
        wait(packet_in.data_written_event());
        Packet pkt = packet_in.read();
        
        // 包进入 TX_Queue
        tx_queue_.push(pkt);
        
        // 更新峰值
        if (static_cast<int>(tx_queue_.size()) > max_queue_depth_) {
            max_queue_depth_ = tx_queue_.size();
        }
        
        // 输出当前队列深度（供 Arbiter 感知）
        tx_queue_depth.write(static_cast<int>(tx_queue_.size()));
        
        // 模拟串行化延迟（20 ns）
        wait(T_SERIALIZE_NS, sc_time_stamp());
        
        // 包离开 TX_Queue，进入物理链路
        tx_queue_.pop();
        
        // 更新队列深度
        tx_queue_depth.write(static_cast<int>(tx_queue_.size()));
        
        // 发送到交换机
        packet_out.write(pkt);
    }
}

#endif // HOST_NIC_H
