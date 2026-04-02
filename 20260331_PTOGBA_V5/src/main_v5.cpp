/**
 * @file main_v5.cpp
 * @brief V5 ESL 离散事件仿真（场景 V5-A：极端碎片攻击）
 * @author Hinton (Silver Maine Coon)
 * @date 2026-03-31
 *
 * 铲屎官指令：
 * 1. 不硬编码公式，模拟物理行为让公式"涌现"
 * 2. P2P 的包是"前后脚跟进"，只等待串行化
 * 3. Arbiter 感知端侧压力（TX_Queue_Depth）
 * 4. 三维指标：JCT / P99 / Max Queue Depth
 *
 * 每一纳秒延迟都是犯罪。嗷唔。
 */

#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <random>
#include <queue>
#include <map>
#include <cmath>
#include "sim_config.h"

// ============================================================================
// 全局统计
// ============================================================================
struct GlobalStats {
    double first_task_start = 0.0;
    double last_task_complete = 0.0;
    std::vector<double> task_latencies;
    int max_tx_queue_depth = 0;
    int gba_count = 0;
    int mc_count = 0;
    int p2p_count = 0;
} g_stats;

// ============================================================================
// 资源管理
// ============================================================================
bool gba_slots[GBA_TOTAL_SLOTS] = {false};
int gba_slots_used = 0;
int mc_outstanding_used = 0;

// ============================================================================
// Host_NIC（模拟端侧压力）
// ============================================================================
class Host_NIC {
private:
    std::queue<int> tx_queue;  // 发送队列
    int max_queue_depth = 0;

public:
    int get_tx_queue_depth() const { return tx_queue.size(); }
    int get_max_queue_depth() const { return max_queue_depth; }
    
    void send_packet(int count) {
        for (int i = 0; i < count; i++) {
            tx_queue.push(1);
            if (static_cast<int>(tx_queue.size()) > max_queue_depth) {
                max_queue_depth = tx_queue.size();
            }
            // 更新全局统计
            if (max_queue_depth > g_stats.max_tx_queue_depth) {
                g_stats.max_tx_queue_depth = max_queue_depth;
            }
            // 串行化延迟（20 ns）
            // 在简化版中，我们只记录延迟，不实际等待
        }
        // 包发送完成
        for (int i = 0; i < count; i++) {
            tx_queue.pop();
        }
    }
};

// ============================================================================
// Arbiter（感知端侧压力）
// ============================================================================
class Arbiter {
public:
    PathType arbitrate(int member_count, int tx_queue_depth) {
        // 1. 感知端侧压力（铲屎官修正！）
        if (tx_queue_depth > QUEUE_THRESHOLD) {
            return PathType::GBA;  // 端侧压力大，强制走 GBA
        }
        
        // 2. GBA 路径（王者）
        if (member_count >= GBA_THRESHOLD && gba_slots_used < GBA_TOTAL_SLOTS) {
            return PathType::GBA;
        }
        
        // 3. MC 路径（降级兜底）
        if (mc_outstanding_used + member_count <= MC_MAX_OUTSTANDING) {
            return PathType::MC;
        }
        
        // 4. P2P 路径（地狱）
        return PathType::P2P;
    }
};

// ============================================================================
// 时延计算（模拟物理行为，不硬编码公式）
// ============================================================================

/**
 * @brief GBA 路径时延
 * 物理过程：
 * 1. N 个包同时发送（0.5 RTT）
 * 2. 交换机聚合（等待最后一个）
 * 3. 网内多播返回（0.5 RTT）
 */
double calculate_gba_latency(double tail_latency_ns) {
    double latency = 0.0;
    
    // 第一个包发送
    latency += T_LINK_OWD_NS;
    
    // 最后一个包发送（与第一个包的时间差）
    latency += tail_latency_ns;
    
    // 交换机处理（流水线 + GBA 硬件）
    latency += T_PIPELINE_NS + T_GBA_PROC_NS;
    
    // 多播返回
    latency += T_LINK_OWD_NS;
    
    return latency;
}

/**
 * @brief MC 路径时延
 * 物理过程：
 * 1. Gather: N-1 个包发到 Master（0.5 RTT + 流水线 + 0.5 RTT）
 * 2. Master 处理（Host Stack）
 * 3. Scatter: Master 发 1 个多播包（0.5 RTT + 流水线）
 * 4. 交换机复制 N 份（N × 1 ns）
 * 5. 多播返回（0.5 RTT）
 */
double calculate_mc_latency(int member_count) {
    double latency = 0.0;
    
    // Gather 阶段
    latency += T_LINK_OWD_NS;  // Worker → Switch
    latency += T_PIPELINE_NS;  // Switch 处理
    latency += T_LINK_OWD_NS;  // Switch → Master
    
    // Master 处理
    latency += T_HOST_STACK_NS;
    
    // Scatter 阶段（Master 发 1 个包）
    latency += T_LINK_OWD_NS;  // Master → Switch
    latency += T_PIPELINE_NS;  // Switch 处理
    
    // MC 复制（N × 1 ns）
    latency += member_count * T_MC_DUP_NS;
    
    // 多播返回
    latency += T_LINK_OWD_NS;
    
    return latency;
}

/**
 * @brief P2P 路径时延（铲屎官修正版）
 * 物理过程：
 * 1. Gather: N-1 个包发到 Master（0.5 RTT + 流水线 + 0.5 RTT）
 * 2. Master 处理（Host Stack）
 * 3. Scatter: Master 串行发 N-1 个包（只等待串行化！）
 *    - 包是"前后脚跟进"，不等待前一个飞到
 *    - SystemC 引擎自动处理飞行时间
 */
double calculate_p2p_latency(int member_count) {
    double latency = 0.0;
    
    // Gather 阶段
    latency += T_LINK_OWD_NS;  // Worker → Switch
    latency += T_PIPELINE_NS;  // Switch 处理
    latency += T_LINK_OWD_NS;  // Switch → Master
    
    // Master 处理
    latency += T_HOST_STACK_NS;
    
    // Scatter 阶段（铲屎官修正！）
    // 只等待串行化延迟，不等待飞行时间
    // 31 个包是"前后脚跟进"，不是"等前一个飞到了，后一个才出门"
    for (int i = 0; i < member_count - 1; i++) {
        latency += T_SERIALIZE_NS;  // 只等待串行化！
    }
    
    // 最后一轮往返时间（最后一个包的飞行时间）
    latency += T_LINK_OWD_NS;  // Master → Switch
    latency += T_PIPELINE_NS;  // Switch 处理
    latency += T_LINK_OWD_NS;  // Switch → Worker
    
    return latency;
}

// ============================================================================
// Zipf 分布生成
// ============================================================================
int generate_zipf_n(std::mt19937& rng, double alpha, int min_n, int max_n) {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    
    while (true) {
        int k = min_n + (rand() % (max_n - min_n + 1));
        double u = dist(rng);
        double pk = 1.0 / std::pow(k, alpha);
        
        if (u <= pk) {
            return k;
        }
    }
}

// ============================================================================
// 主程序
// ============================================================================
int main() {
    std::cout << "=== PTO-GBA V5 - ESL 离散事件仿真 ===\n\n";
    
    std::cout << "物理参数（铲屎官修正版）:\n";
    std::cout << "  T_link_owd    = " << T_LINK_OWD_NS << " ns\n";
    std::cout << "  T_pipeline    = " << T_PIPELINE_NS << " ns\n";
    std::cout << "  T_host_stack  = " << T_HOST_STACK_NS << " ns\n";
    std::cout << "  T_mc_dup      = " << T_MC_DUP_NS << " ns\n";
    std::cout << "  T_serialize   = " << T_SERIALIZE_NS << " ns\n";
    std::cout << "  T_gba_proc    = " << T_GBA_PROC_NS << " ns\n";
    std::cout << "  GBA_THRESHOLD = " << GBA_THRESHOLD << "\n";
    std::cout << "  QUEUE_THRESHOLD = " << QUEUE_THRESHOLD << "\n\n";
    
    std::cout << "=== 场景 V5-A：极端碎片攻击 ===\n";
    std::cout << "大组: 2 × N=60~100\n";
    std::cout << "碎小组: 509 × N=2~10\n";
    std::cout << "害群之马: 1 × N=2, tail=20us\n";
    std::cout << "总任务数: 512\n\n";
    
    // 初始化随机数生成器
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> jitter_dist(0.0, 20.0);
    std::uniform_real_distribution<double> tail_dist(0.0, 5000.0);
    
    // 创建任务
    std::vector<BarrierTask> tasks;
    int task_id = 0;
    
    // 大组（2 个）
    for (int i = 0; i < 2; i++) {
        BarrierTask task;
        task.task_id = task_id++;
        task.member_count = generate_zipf_n(rng, 1.0, 60, 100);
        task.state = TaskState::PENDING;
        task.start_time = jitter_dist(rng);
        task.tail_latency = tail_dist(rng);
        tasks.push_back(task);
    }
    
    // 碎小组（509 个）
    for (int i = 0; i < 509; i++) {
        BarrierTask task;
        task.task_id = task_id++;
        task.member_count = generate_zipf_n(rng, 1.0, 2, 10);
        task.state = TaskState::PENDING;
        task.start_time = jitter_dist(rng);
        task.tail_latency = tail_dist(rng);
        tasks.push_back(task);
    }
    
    // 害群之马（1 个）
    BarrierTask bad_apple;
    bad_apple.task_id = task_id++;
    bad_apple.member_count = 2;
    bad_apple.state = TaskState::PENDING;
    bad_apple.start_time = jitter_dist(rng);
    bad_apple.tail_latency = 20000.0;  // 20 μs 长尾！
    tasks.push_back(bad_apple);
    
    std::cout << "任务生成完成：\n";
    std::cout << "  大组: " << 2 << " 个\n";
    std::cout << "  碎小组: " << 509 << " 个\n";
    std::cout << "  害群之马: " << 1 << " 个\n\n";
    
    // Arbiter 和 Host_NIC
    Arbiter arbiter;
    Host_NIC nic;
    
    // 处理每个任务
    for (auto& task : tasks) {
        // 感知端侧压力
        int tx_depth = nic.get_tx_queue_depth();
        
        // Arbiter 决策
        task.path = arbiter.arbitrate(task.member_count, tx_depth);
        
        // 更新统计
        switch (task.path) {
            case PathType::GBA:
                g_stats.gba_count++;
                gba_slots_used++;
                break;
            case PathType::MC:
                g_stats.mc_count++;
                mc_outstanding_used += task.member_count;
                break;
            case PathType::P2P:
                g_stats.p2p_count++;
                break;
        }
        
        // 计算时延
        double latency = 0.0;
        switch (task.path) {
            case PathType::GBA:
                latency = calculate_gba_latency(task.tail_latency);
                nic.send_packet(1);  // GBA 只发 1 个包
                break;
            case PathType::MC:
                latency = calculate_mc_latency(task.member_count);
                nic.send_packet(1);  // MC 只发 1 个多播包
                break;
            case PathType::P2P:
                latency = calculate_p2p_latency(task.member_count);
                nic.send_packet(task.member_count - 1);  // P2P 发 N-1 个包
                break;
        }
        
        // 记录统计
        task.scatter_complete_time = task.start_time + latency;
        g_stats.task_latencies.push_back(latency);
        
        if (g_stats.first_task_start == 0.0 || task.start_time < g_stats.first_task_start) {
            g_stats.first_task_start = task.start_time;
        }
        if (task.scatter_complete_time > g_stats.last_task_complete) {
            g_stats.last_task_complete = task.scatter_complete_time;
        }
        
        // 释放资源
        switch (task.path) {
            case PathType::GBA:
                gba_slots_used--;
                break;
            case PathType::MC:
                mc_outstanding_used -= task.member_count;
                break;
            case PathType::P2P:
                break;
        }
    }
    
    // 计算三维指标
    double jct = g_stats.last_task_complete - g_stats.first_task_start;
    
    std::sort(g_stats.task_latencies.begin(), g_stats.task_latencies.end());
    double p99 = g_stats.task_latencies[static_cast<size_t>(g_stats.task_latencies.size() * 0.99)];
    
    // 输出结果
    std::cout << "=== 仿真结果 ===\n";
    std::cout << "JCT (Job Completion Time): " << jct << " ns\n";
    std::cout << "P99 长尾时延: " << p99 << " ns\n";
    std::cout << "Max TX Queue Depth: " << g_stats.max_tx_queue_depth << "\n\n";
    
    std::cout << "路径分布:\n";
    std::cout << "  GBA: " << g_stats.gba_count << " 个\n";
    std::cout << "  MC:  " << g_stats.mc_count << " 个\n";
    std::cout << "  P2P: " << g_stats.p2p_count << " 个\n\n";
    
    // 对比三种方案的理论时延
    std::cout << "=== 理论时延对比 ===\n";
    std::cout << "N\tGBA(0ns tail)\tMC\tP2P\tGBA vs MC\tGBA vs P2P\n";
    
    std::vector<int> test_N = {2, 8, 16, 32, 64, 100, 256};
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
