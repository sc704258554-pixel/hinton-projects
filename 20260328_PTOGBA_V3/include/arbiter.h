/**
 * @file arbiter.h
 * @brief Hybrid Arbiter 核心仲裁器
 * @author Hinton (Silver Maine Coon)
 * @date 2026-03-27
 *
 * 绝对零死锁。死保大组走 GBA。软件兜底率 = 0%。
 * 每一纳秒延迟都是犯罪。嗷唔。
 */

#ifndef ARBITER_H
#define ARBITER_H

#include <systemc>
#include <queue>
#include <vector>
#include <map>
#include "types.h"
#include "sim_config.h"
#include "latency_model.h"

SC_MODULE(HybridArbiter) {
    // ============================================================================
    // 端口 (Ports)
    // ============================================================================
    // 请求输入端口
    sc_core::sc_fifo_in<BarrierRequest> request_in;

    // 响应输出端口
    sc_core::sc_fifo_out<BarrierResponse> response_out;

    // 时钟和复位
    sc_core::sc_in<bool> clk;
    sc_core::sc_in<bool> reset;

    // ============================================================================
    // 内部状态 (Internal State)
    // ============================================================================
private:
    // 资源状态
    ResourceStatus resource_status_;

    // 等待队列 (按优先级分组)
    std::priority_queue<
        BarrierRequest,
        std::vector<BarrierRequest>,
        bool(*)(const BarrierRequest&, const BarrierRequest&)
    > high_priority_queue_;
    std::queue<BarrierRequest> normal_queue_;

    // 大组预留计数器
    uint32_t gba_reserved_slots_;

    // 统计信息
    uint64_t total_requests_;
    uint64_t gba_requests_;
    uint64_t mc_requests_;
    uint64_t host_requests_;
    uint64_t timeout_count_;

    // ============================================================================
    // 核心方法 (Core Methods)
    // ============================================================================
public:
    // 主仲裁线程
    void arbitration_process();

    // 超时监控线程
    void timeout_monitor();

    // 资源状态更新
    void update_resource_status();

    // ============================================================================
    // V2.2 新增: 并发持有资源方法
    // ============================================================================
    /**
     * @brief 并发持有资源（独立线程）
     * @param request 请求
     * @param path 分配的路径
     * @param response 响应对象（由 spawned 线程填充并发送）
     */
    void hold_and_release_resource(BarrierRequest request, PathType path, BarrierResponse response);

    // ============================================================================
    // 仲裁决策方法 (Arbitration Decision Methods)
    // ============================================================================
private:
    /**
     * @brief Banker's 安全检查
     * @param request 请求
     * @param path 拟分配路径
     * @return 是否安全
     */
    bool is_safe_to_allocate(const BarrierRequest& request, PathType path);

    /**
     * @brief 路径选择决策
     * @param request 请求
     * @return 分配的路径
     */
    PathType select_path(const BarrierRequest& request);

    /**
     * @brief 分配资源
     * @param request 请求
     * @param path 路径
     * @return 是否成功
     */
    bool allocate_resource(const BarrierRequest& request, PathType path);

    /**
     * @brief 释放资源
     * @param request 请求
     * @param path 路径
     */
    void release_resource(const BarrierRequest& request, PathType path);

    /**
     * @brief 检查 GBA slot 是否可用 (考虑预留)
     * @return 是否有可用 slot
     */
    bool has_gba_slot_available();

    /**
     * @brief 检查 MC capacity 是否足够
     * @param n 组大小
     * @return 是否有足够 capacity
     */
    bool has_mc_capacity(uint16_t n);

    /**
     * @brief 动态选择路径 (中间组 16 <= N < 64)
     * @param request 请求
     * @return 路径
     */
    PathType dynamic_select(const BarrierRequest& request);

    /**
     * @brief 处理超时降级
     * @param request 请求
     */
    void handle_timeout(BarrierRequest& request);

    // ============================================================================
    // 辅助方法 (Helper Methods)
    // ============================================================================
private:
    // 比较函数 (用于优先级队列)
    static bool compare_priority(
        const BarrierRequest& a,
        const BarrierRequest& b
    ) {
        // 优先级高的在前，相同优先级时大组优先
        if (a.priority != b.priority) {
            return a.priority < b.priority;
        }
        return a.group_size < b.group_size;
    }

public:
    // 构造函数
    SC_CTOR(HybridArbiter)
        : high_priority_queue_(compare_priority)
    {
        SC_THREAD(arbitration_process);
        sensitive << clk.pos();
        async_reset_signal_is(reset, true);

        SC_THREAD(timeout_monitor);
        sensitive << clk.pos();
        async_reset_signal_is(reset, true);

        // 初始化状态
        gba_reserved_slots_ = 0;
        total_requests_ = 0;
        gba_requests_ = 0;
        mc_requests_ = 0;
        host_requests_ = 0;
        timeout_count_ = 0;
    }

    // 获取统计信息
    uint64_t get_total_requests() const { return total_requests_; }
    uint64_t get_gba_requests() const { return gba_requests_; }
    uint64_t get_mc_requests() const { return mc_requests_; }
    uint64_t get_host_requests() const { return host_requests_; }
    uint64_t get_timeout_count() const { return timeout_count_; }
    double get_host_fallback_rate() const {
        return total_requests_ > 0
            ? static_cast<double>(host_requests_) / total_requests_
            : 0.0;
    }
    const ResourceStatus& get_resource_status() const { return resource_status_; }
};

#endif // ARBITER_H
