/**
 * @file stimulus_gen.h
 * @brief V5 激励生成器
 * @author Hinton (Silver Maine Coon)
 * @date 2026-03-31
 *
 * 场景 V5-A：512 个任务（极端碎片攻击）
 * 场景 V5-B：1024 个任务（Pipeline 多层并发）
 * 每一纳秒延迟都是犯罪。嗷唔。
 */

#ifndef STIMULUS_GEN_H
#define STIMULUS_GEN_H

#include <vector>
#include <random>
#include <cmath>
#include "types.h"
#include "sim_config.h"

// ============================================================================
// 激励生成器
// ============================================================================
class StimulusGenerator {
public:
    /**
     * @brief 生成场景 V5-A（极端碎片攻击）
     * @return 任务列表
     */
    static std::vector<BarrierTask> generate_scenario_a() {
        std::vector<BarrierTask> tasks;
        
        // 2 个大组（N=60~100）
        for (int i = 0; i < 2; i++) {
            BarrierTask task;
            task.task_id = i;
            task.member_count = generate_zipf_member(1.0, 60, 100);
            task.path = PathType::GBA;
            task.state = TaskState::PENDING;
            task.start_time = 0.0;
            task.tail_latency = generate_tail_latency(0, 5000);
            tasks.push_back(task);
        }
        
        // 509 个碎小组（N=2~10）
        for (int i = 2; i < 511; i++) {
            BarrierTask task;
            task.task_id = i;
            task.member_count = generate_zipf_member(1.0, 2, 10);
            task.path = PathType::GBA;
            task.state = TaskState::PENDING;
            task.start_time = 0.0;
            task.tail_latency = generate_tail_latency(0, 5000);
            tasks.push_back(task);
        }
        
        // 1 个害群之马（N=2, tail=20μs)
        BarrierTask task;
        task.task_id = 511;
        task.member_count = 2;
        task.path = PathType::GBA;
        task.state = TaskState::PENDING;
        task.start_time = 0.0;
        task.tail_latency = 20000.0;  // 20 μs 的长尾
        tasks.push_back(task);
        
        return tasks;
    }
    
    /**
     * @brief 生成场景 V5-B（Pipeline 多层并发）
     * @return 任务列表
     */
    static std::vector<BarrierTask> generate_scenario_b() {
        std::vector<BarrierTask> tasks;
        int task_id = 0;
        
        // 4 层并发（Layer 15, 30, 45, 60）
        for (int layer = 0; layer < 4; layer++) {
            // 每层 256 个任务
            for (int i = 0; i < 256; i++) {
                BarrierTask task;
                task.task_id = task_id++;
                task.member_count = generate_zipf_member(1.0, 2, 100);
                task.path = PathType::GBA;
                task.state = TaskState::PENDING;
                task.start_time = layer * 100.0;  // 分波出发
                task.tail_latency = generate_tail_latency(0, 5000);
                tasks.push_back(task);
            }
        }
        
        return tasks;
    }
    
private:
    static std::mt19937 rng_;
    
    /**
     * @brief 生成 Zipf 分布的 member 数
     * @param alpha Zipf 参数
     * @param min_n 最小 N
     * @param max_n 最大 N
     * @return Zipf 分布的随机 member 数
     */
    static int generate_zipf_member(double alpha, int min_n, int max_n) {
        // 拒绝采样
        while (true) {
            int k = min_n + (rand() % (max_n - min_n + 1));
            double u = static_cast<double>(rand()) / RAND_MAX;
            double pk = 1.0 / std::pow(k, alpha);
            
            if (u <= pk) {
                return k;
            }
        }
    }
    
    /**
     * @brief 生成尾包到达时间差
     * @param min_ns 最小值
     * @param max_ns 最大值
     * @return 尾包到达时间差（ns）
     */
    static double generate_tail_latency(double min_ns, double max_ns) {
        return min_ns + static_cast<double>(rand()) / RAND_MAX * (max_ns - min_ns);
    }
};

#endif // STIMULUS_GEN_H
