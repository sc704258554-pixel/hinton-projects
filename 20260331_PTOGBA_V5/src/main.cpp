/**
 * @file main.cpp
 * @brief V5 主程序入口
 * @author Hinton (Silver Maine Coon)
 * @date 2026-03-31
 *
 * 主程序入口：启动仿真 + 输出结果
 * 每一纳秒延迟都是犯罪。嗷唔。
 */

#include <systemc>
#include "sim_config.h"
#include "types.h"
#include "host_nic.h"
#include "switch_port.h"
#include "gba_engine.h"
#include "mc_engine.h"
#include "arbiter.h"
#include "system_monitor.h"
#include "stimulus_gen.h"

#include <iostream>
#include <iomanip>

// ============================================================================
// 全局仿真参数
// ============================================================================
sc_signal<Packet> packet_to_switch("packet_to_switch");
sc_signal<Packet> packet_from_switch("packet_from_switch");
sc_signal<int> tx_queue_depth("tx_queue_depth");
sc_signal<int> gba_slots_used("gba_slots_used");
sc_signal<int> mc_outstanding_used("mc_outstanding_used");
sc_signal<bool> gba_slot_full("gba_slot_full");
sc_signal<bool> mc_outstanding_full("mc_outstanding_full");
sc_signal<PathType> selected_path("selected_path");
sc_signal<BarrierTask> task_request("task_request");
sc_signal<BarrierTask> task_completed("task_completed");

// ============================================================================
// 主程序
// ============================================================================
int sc_main(int argc, char* argv[]) {
    // 实例化模块
    Host_NIC host_nic("host_nic");
    Switch_Port switch_port("switch_port");
    GBA_Engine gba_engine("gba_engine");
    MC_Engine mc_engine("mc_engine");
    Arbiter arbiter("arbiter");
    System_Monitor monitor("monitor");
    
    // 连接信号
    host_nic.packet_out(packet_to_switch);
    switch_port.packet_in(packet_to_switch);
    switch_port.packet_to_gba(packet_to_switch);  // 简化：直接连到 GBA
    
    gba_engine.packet_in(packet_to_switch);
    gba_engine.packet_out(packet_from_switch);
    
    host_nic.packet_in(packet_from_switch);
    host_nic.tx_queue_depth(tx_queue_depth);
    
    gba_engine.slots_used(gba_slots_used);
    gba_engine.slot_full(gba_slot_full);
    
    mc_engine.outstanding_used(mc_outstanding_used);
    mc_engine.outstanding_full(mc_outstanding_full);
    
    arbiter.task_request(task_request);
    arbiter.tx_queue_depth(tx_queue_depth);
    arbiter.gba_slots_used(gba_slots_used);
    arbiter.mc_outstanding_used(mc_outstanding_used);
    arbiter.gba_slot_full(gba_slot_full);
    arbiter.mc_outstanding_full(mc_outstanding_full);
    arbiter.selected_path(selected_path);
    
    monitor.task_completed(task_completed);
    monitor.tx_queue_depth(tx_queue_depth);
    
    // 生成激励
    std::cout << "╔════════════════════════════════════════════════════════╗\n";
    std::cout << "║     PTO-GBA V5 - ESL 离散事件仿真           ║\n";
    std::cout << "║     Hinton (Silver Maine Coon)                           ║\n";
    std::cout << "╚════════════════════════════════════════════════════════╝\n\n";
    std::cout << "\n物理参数（铲屎官修正版）:\n";
    std::cout << "  T_link_owd    = " << std::setw(6) << T_LINK_OWD_NS << " ns\n";
    std::cout << "  T_pipeline    = " << std::setw(6) << T_PIPELINE_NS << " ns\n";
    std::cout << "  T_host_stack  = " << std::setw(6) << T_HOST_STACK_NS << " ns\n";
    std::cout << "  T_mc_dup      = " << std::setw(6) << T_MC_DUP_NS << " ns\n";
    std::cout << "  T_serialize   = " << std::setw(6) << T_SERIALIZE_NS << " ns\n";
    std::cout << "  T_gba_proc    = " << std::setw(6) << T_GBA_PROC_NS << " ns\n";
    
    // 运行场景 V5-A
    std::cout << "\n=== 场景 V5-A：极端碎片攻击（512 任务）===\n";
    auto tasks_a = StimulusGenerator::generate_scenario_a();
    for (const auto& task : tasks_a) {
        task_request.write(task);
        wait(1, SC_NS);  // 微观抖动
    }
    
    // 等待仿真完成
    wait(10000, SC_NS);  // 10 μs
    
    // 输出结果
    SimulationStats stats = monitor.get_stats();
    
    std::cout << "\n=== 仿真结果 ===\n";
    std::cout << "JCT: " << std::fixed << std::setprecision(2) 
              << (stats.last_task_complete_time - stats.first_task_start_time) << " ns\n";
    
    // 计算 P99
    std::sort(stats.task_latencies.begin(), stats.task_latencies.end());
    double p99 = stats.task_latencies[static_cast<size_t>(stats.task_latencies.size() * 0.99)];
    
    std::cout << "P99: " << std::fixed << std::setprecision(2) << p99 << " ns\n";
    std::cout << "Max TX Queue Depth: " << stats.max_tx_queue_depth << "\n";
    std::cout << "路径分布: GBA=" << stats.gba_count 
              << ", MC=" << stats.mc_count 
              << ", P2P=" << stats.p2p_count << "\n";
    
    std::cout << "\n=== 测试完成 ===\n";
    std::cout << "看在猫条的份上帮你搞定。嗷唔 🐱\n";
    
    return 0;
}
