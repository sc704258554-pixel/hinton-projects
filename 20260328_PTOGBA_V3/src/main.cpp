/**
 * @file main.cpp
 * @brief PTO-GBA ESL 仿真主入口
 * @author Hinton (Silver Maine Coon)
 * @date 2026-03-27
 *
 * PTO-GBA 端网协同与 ESL 仿真收益白皮书 V1.0
 * 每一纳秒延迟都是犯罪。嗷唔。
 */

#include <systemc>
#include <iostream>
#include "sim_config.h"
#include "types.h"
#include "latency_model.h"
#include "arbiter.h"
#include "scenario_gen.h"
#include "stats.h"

// ============================================================================
// 仿真顶层模块
// ============================================================================
SC_MODULE(TopModule) {
    // 信号
    sc_core::sc_clock clk;
    sc_core::sc_signal<bool> reset;

    // FIFO 通道
    sc_core::sc_fifo<BarrierRequest> request_fifo;
    sc_core::sc_fifo<BarrierResponse> response_fifo;

    // 子模块
    ScenarioGenerator* generator;
    HybridArbiter* arbiter;
    StatsCollector* collector;

    // 构造函数
    SC_CTOR(TopModule)
        : clk("clk", 1, sc_core::SC_NS),  // 1GHz 时钟
          request_fifo(1024),
          response_fifo(1024)
    {
        // 实例化场景生成器
        generator = new ScenarioGenerator("generator");
        generator->request_out(request_fifo);
        generator->clk(clk);
        generator->reset(reset);

        // 实例化仲裁器
        arbiter = new HybridArbiter("arbiter");
        arbiter->request_in(request_fifo);
        arbiter->response_out(response_fifo);
        arbiter->clk(clk);
        arbiter->reset(reset);

        // 实例化统计收集器
        collector = new StatsCollector("collector");
        collector->response_in(response_fifo);
        collector->clk(clk);
        collector->reset(reset);
    }

    // 析构函数
    ~TopModule() {
        delete generator;
        delete arbiter;
        delete collector;
    }
};

// ============================================================================
// 主函数
// ============================================================================
int sc_main(int argc, char* argv[]) {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║         PTO-GBA 端网协同 ESL 仿真框架 V1.0                  ║\n";
    std::cout << "║         Hybrid Arbiter with Dynamic Latency Model           ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";

    // 打印配置信息
    std::cout << "📋 仿真配置\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "  交换机数量:       " << NUM_SWITCHES << "\n";
    std::cout << "  总 GBA Slots:     " << TOTAL_GBA_SLOTS << "\n";
    std::cout << "  总 MC Outstanding: " << TOTAL_MC_OUTSTANDING << "\n";
    std::cout << "  GBA 阈值 (N≥):    " << GBA_THRESHOLD_N << "\n";
    std::cout << "  MC 阈值 (N<):     " << MC_THRESHOLD_N << "\n";
    std::cout << "  GBA 预留比例:     " << (GBA_RESERVE_RATIO * 100) << "%\n";
    std::cout << "\n";

    std::cout << "⏱️  动态时延 Base 值\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "  GBA Base:    " << GBA_BASE_DELAY_NS << " ns\n";
    std::cout << "  MC Base:     " << MC_BASE_DELAY_NS << " ns\n";
    std::cout << "  HOST Base:   " << HOST_BASE_DELAY_NS << " ns\n";
    std::cout << "  SRAM Lookup: " << SRAM_LOOKUP_NS << " ns\n";
    std::cout << "  PCIe Overhead: " << PCIE_OVERHEAD_NS << " ns\n";
    std::cout << "\n";

    std::cout << "🎬 V2.0 极限激励场景\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "  场景 A (MoE 常态):   2,000 任务, 10% Straggler\n";
    std::cout << "  场景 B (MoE 洪峰):   5,000 任务, 20% Straggler\n";
    std::cout << "  场景 C (恶意压测):  10,000 任务, 30% Straggler\n";
    std::cout << "\n";
    std::cout << "  当前选择: 场景 A (MoE 常态)\n";
    std::cout << "\n";

    std::cout << "🚀 开始仿真...\n\n";

    // 创建顶层模块
    TopModule top("top");

    // 复位脉冲
    top.reset.write(true);
    sc_core::sc_start(10, sc_core::SC_NS);
    top.reset.write(false);

    // 运行仿真
    sc_core::sc_start(SIM_TIME_NS, sc_core::SC_NS);

    // V3: 等待所有 spawned 线程完成
    // 资源持有时间最长约为 100us × (1 + 256/64) = 500us
    // 预留 10ms 确保所有异步任务完成
    sc_core::sc_start(10000000, sc_core::SC_NS);  // 10ms

    // 最终统计
    std::cout << "\n📊 仿真结束，生成报告...\n";
    top.collector->finalize();

    std::cout << "\n✅ ESL 仿真完成。嗷唔 🐱\n\n";

    return 0;
}
