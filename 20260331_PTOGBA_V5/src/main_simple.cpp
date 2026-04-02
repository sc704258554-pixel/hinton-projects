/**
 * @file main_simple.cpp
 * @brief V5 简化版主程序
 * @author Hinton (Silver Maine Coon)
 * @date 2026-03-31
 */

#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include "sim_config.h"

double calculate_gba_latency(double tail_latency_ns) {
    double t1 = T_LINK_OWD_NS;
    double tN = t1 + tail_latency_ns;
    double t_switch = T_PIPELINE_NS + T_GBA_PROC_NS;
    double t_return = T_LINK_OWD_NS;
    return tN - t1 + t_switch + t_return;
}

double calculate_mc_latency(int member_count) {
    double t_gather = T_LINK_OWD_NS + T_PIPELINE_NS + T_LINK_OWD_NS;
    double t_master = T_HOST_STACK_NS;
    double t_scatter_send = T_LINK_OWD_NS + T_PIPELINE_NS;
    double t_mc_copy = member_count * T_MC_DUP_NS;
    double t_scatter_return = T_LINK_OWD_NS;
    return t_gather + t_master + t_scatter_send + t_mc_copy + t_scatter_return;
 }

double calculate_p2p_latency(int member_count) {
    double t_gather = T_LINK_OWD_NS + T_PIPELINE_NS + T_LINK_OWD_NS;
    double t_master = T_HOST_STACK_NS;
    double t_scatter_total = 0.0;
    for (int i = 0; i < member_count - 1; i++) {
        t_scatter_total += T_SERIALIZE_NS + T_LINK_OWD_NS + T_PIPELINE_NS + T_LINK_OWD_NS;
    }
    return t_gather + t_master + t_scatter_total;
}

int main() {
    std::cout << "=== PTO-GBA V5 - 时延公式验证 ===\n\n";
    std::cout << "\n物理参数（铲屎官修正版）:\n";
    std::cout << "  T_link_owd    = " << T_LINK_OWD_NS << " ns\n";
    std::cout << "  T_pipeline    = " << T_PIPELINE_NS << " ns\n";
    std::cout << "  T_host_stack  = " << T_HOST_STACK_NS << " ns\n";
    std::cout << "  T_mc_dup      = " << T_MC_DUP_NS << " ns\n";
    std::cout << "  T_serialize   = " << T_SERIALIZE_NS << " ns\n";
    std::cout << "  T_gba_proc   = " << T_GBA_PROC_NS << " ns\n";
    
    std::cout << "\n=== 理论公式 ===\n";
    std::cout << "GBA: E2E = 2*T_link + T_pipeline + T_gba_proc + tail\n";
    std::cout << "MC:  E2E = 4*T_link + 2*T_pipeline + T_host_stack + N*T_mc_dup\n";
    std::cout << "P2P: E2E = 4*T_link + 2*T_pipeline + T_host_stack + (N-1)*T_serialize\n";
    
    std::cout << "\n=== 验证结果 ===\n";
    
    std::vector<int> test_N = {2, 8, 16, 32, 64, 100, 256};
    
    std::cout << "N\tGBA(0ns tail)\tMC\tP2P\tGBA vs MC\tGBA vs P2P\n";
    for (int N : test_N) {
        double gba = calculate_gba_latency(0);
        double mc = calculate_mc_latency(N);
        double p2p = calculate_p2p_latency(N);
        
        std::cout << N << "\t" << gba << "\t\t" << mc << "\t" << p2p 
                  << "\t" << (mc / gba) << "x\t" << (p2p / gba) << "x\n";
    }
    
    std::cout << "\n=== 完成验证！看在猫条的份上帮你搞定。嗷唔 ===\n";
    std::cout << "\n等待傅立叶 Code Review！\n";
    
    return 0;
}
