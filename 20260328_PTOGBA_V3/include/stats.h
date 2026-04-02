/**
 * @file stats.h
 * @brief 仿真统计模块
 * @author Hinton (Silver Maine Coon)
 * @date 2026-03-27
 *
 * 数据驱动闭环：自动打印动态 E2E 收益
 * 每一纳秒延迟都是犯罪。嗷唔。
 */

#ifndef STATS_H
#define STATS_H

#include <systemc>
#include <vector>
#include <map>
#include <fstream>
#include "types.h"
#include "sim_config.h"

/**
 * @struct SimulationStats
 * @brief 仿真统计数据
 */
struct SimulationStats {
    // V3 新增: 端到端时延
    double min_start_time_ns;
    double max_completion_time_ns;
    double e2e_latency_ns;

    // V3 新增: 按波次统计
    std::map<uint8_t, double> wave_min_start_ns;
    std::map<uint8_t, double> wave_max_completion_ns;
    std::map<uint8_t, double> wave_e2e_ns;
    std::map<uint8_t, uint64_t> wave_task_count;

    // 按组大小分类的 E2E 平均时延
    double avg_latency_small;    // N < 16
    double avg_latency_medium;   // 16 <= N < 64
    double avg_latency_large;    // N >= 64

    // 按组大小分类的请求计数
    uint64_t count_small;
    uint64_t count_medium;
    uint64_t count_large;

    // 资源利用率
    double gba_slot_utilization;
    double mc_outstanding_utilization;

    // 路径分布
    uint64_t gba_count;
    uint64_t mc_count;
    uint64_t host_count;

    // 软件兜底率 (目标: 0%)
    double host_fallback_rate;

    // 死锁/超时次数
    uint32_t timeout_count;
    uint32_t deadlock_count;

    // 总请求数
    uint64_t total_requests;

    // 仿真时间
    double sim_time_ns;

    // 默认构造
    SimulationStats()
        : min_start_time_ns(1e18), max_completion_time_ns(0), e2e_latency_ns(0),
          avg_latency_small(0.0), avg_latency_medium(0.0), avg_latency_large(0.0),
          count_small(0), count_medium(0), count_large(0),
          gba_slot_utilization(0.0), mc_outstanding_utilization(0.0),
          gba_count(0), mc_count(0), host_count(0),
          host_fallback_rate(0.0),
          timeout_count(0), deadlock_count(0),
          total_requests(0), sim_time_ns(0.0) {}

    // 计算平均时延
    void calculate_averages();

    // 计算兜底率
    void calculate_fallback_rate();

    // 计算端到端时延
    void calculate_e2e_latency();

    // V3: 计算每波的端到端时延
    void calculate_wave_e2e();

    // 打印报告
    void print_report() const;

    // 保存到文件
    void save_to_file(const std::string& path) const;
};

/**
 * @class StatsCollector
 * @brief 统计收集器 (SystemC 模块)
 */
SC_MODULE(StatsCollector) {
    // ============================================================================
    // 端口 (Ports)
    // ============================================================================
    sc_core::sc_fifo_in<BarrierResponse> response_in;
    sc_core::sc_in<bool> clk;
    sc_core::sc_in<bool> reset;

    // ============================================================================
    // 内部状态 (Internal State)
    // ============================================================================
private:
    SimulationStats stats_;

    // 时延记录 (按类别)
    std::vector<double> latencies_small_;
    std::vector<double> latencies_medium_;
    std::vector<double> latencies_large_;

    // ============================================================================
    // 核心方法 (Core Methods)
    // ============================================================================
public:
    // 收集线程
    void collect_process();

    // 最终统计
    void finalize();

    // 获取统计数据
    const SimulationStats& get_stats() const { return stats_; }

    // ============================================================================
    // 构造函数
    // ============================================================================
public:
    SC_CTOR(StatsCollector) {
        SC_THREAD(collect_process);
        sensitive << clk.pos();
        async_reset_signal_is(reset, true);
    }
};

#endif // STATS_H
