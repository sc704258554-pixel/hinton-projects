/**
 * @file switch_port.h
 * @brief 交换机端口模块
 * @author Hinton (Silver Maine Coon)
 * @date 2026-03-31
 *
 * 模拟物理行为：流水线延迟
 * 每一纳秒延迟都是犯罪。嗷唔。
 */

#ifndef SWITCH_PORT_H
#define SWITCH_PORT_H

#include <systemc>
#include "types.h"
#include "sim_config.h"

// ============================================================================
// 交换机端口模块
// ============================================================================
SC_MODULE(Switch_Port) {
public:
    // 输入端口：接收来自端侧的包
    sc_in<Packet> packet_in;
    
    // 输出端口：转发到 GBA_Engine 或 MC_Engine
    sc_out<Packet> packet_to_gba;
    sc_out<Packet> packet_to_mc;
    
    // 输出端口：转发回端侧（多播响应）
    sc_out<Packet> packet_to_host;
    
    SC_CTOR(Switch_Port);
    
private:
    void process_packet();
};

// ============================================================================
// 实现
// ============================================================================
SC_CTOR(Switch_Port) {
    SC_METHOD(process_packet);
    sensitive << packet_in;
}

void Switch_Port::process_packet() {
    if (packet_in.event()) {
        Packet pkt = packet_in.read();
        
        // 模拟流水线延迟（500 ns）
        wait(T_PIPELINE_NS, SC_NS);
        
        // 根据任务路径类型转发
        // 这里简化处理：由 GBA_Engine/MC_Engine 决定
        // 实际应该由 Arbiter 指定路径
        packet_to_gba.write(pkt);  // 默认走 GBA
    }
}

#endif // SWITCH_PORT_H
