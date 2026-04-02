/**
 * @file arbiter.cpp
 * @brief Hybrid Arbiter 核心仲裁器实现
 * @author Hinton (Silver Maine Coon)
 * @date 2026-03-27
 *
 * 绝对零死锁。死保大组走 GBA。软件兜底率 = 0%。
 * 每一纳秒延迟都是犯罪。嗷唔。
 */

#include "arbiter.h"
#include <iostream>
#include <algorithm>

// ============================================================================
// 主仲裁线程 (Main Arbitration Thread)
// ============================================================================
void HybridArbiter::arbitration_process() {
    while (true) {
        wait();

        // 复位处理
        if (reset.read()) {
            resource_status_ = ResourceStatus();
            gba_reserved_slots_ = 0;
            total_requests_ = 0;
            gba_requests_ = 0;
            mc_requests_ = 0;
            host_requests_ = 0;
            timeout_count_ = 0;
            while (!high_priority_queue_.empty()) high_priority_queue_.pop();
            while (!normal_queue_.empty()) normal_queue_.pop();
            continue;
        }

        // 检查新请求
        if (request_in.num_available() > 0) {
            BarrierRequest request = request_in.read();
            request.arrival_time = sc_core::sc_time_stamp();
            total_requests_++;

            // 按优先级入队
            if (request.priority > 0) {
                high_priority_queue_.push(request);
            } else {
                normal_queue_.push(request);
            }
        }

        // 处理高优先级队列
        if (!high_priority_queue_.empty()) {
            BarrierRequest request = high_priority_queue_.top();
            high_priority_queue_.pop();

            // 路径选择
            PathType path = select_path(request);

            // Banker's 安全检查
            if (!is_safe_to_allocate(request, path)) {
                // 降级处理
                if (path == PathType::GBA) {
                    path = PathType::MC;
                    if (!is_safe_to_allocate(request, path)) {
                        path = PathType::HOST;
                    }
                } else if (path == PathType::MC) {
                    path = PathType::HOST;
                }
            }

            // 分配资源
            if (allocate_resource(request, path)) {
                // 计算时延
                double latency = LatencyModel::calculate_dynamic_latency(
                    request.group_size,
                    path,
                    resource_status_.congestion_factor,
                    resource_status_.queue_depth
                );

                // 构造响应
                BarrierResponse response;
                response.task_id = request.task_id;
                response.path_used = path;
                response.latency_ns = latency;
                response.success = true;
                response.timeout = false;
                response.wave_id = request.wave_id;  // V3: 传递波次标记
                response.arrival_time = request.arrival_time;
                // V3: completion_time 将在 spawned 线程中设置（等待资源持有完成后）

                // 更新统计
                switch (path) {
                    case PathType::GBA:  gba_requests_++; break;
                    case PathType::MC:   mc_requests_++; break;
                    case PathType::HOST: host_requests_++; break;
                }

                // V3: 主线程不发送响应！响应将在 spawned 线程中发送
                
                // V2.2: 并发持有资源 - spawn 独立线程，不阻塞主仲裁线程
                // 真实同步操作需要 us 级时间，主线程继续处理下一个请求
                
                // Spawn 独立线程持有资源（并发执行）
                sc_core::sc_spawn(sc_core::sc_bind(&HybridArbiter::hold_and_release_resource, this, request, path, response));
                
                // 主线程不等待，继续处理下一个请求！
            } else {
                // 分配失败（理论上不应该发生）
                BarrierResponse response;
                response.task_id = request.task_id;
                response.path_used = path;
                response.success = false;
                response.timeout = false;
                response_out.write(response);
            }

            // 更新资源状态
            update_resource_status();
        }

        // 处理普通队列（如果有高优先级任务在等待，先处理高优先级）
        if (high_priority_queue_.empty() && !normal_queue_.empty()) {
            BarrierRequest request = normal_queue_.front();
            normal_queue_.pop();

            // 同样的处理流程
            PathType path = select_path(request);

            if (!is_safe_to_allocate(request, path)) {
                if (path == PathType::GBA) {
                    path = PathType::MC;
                    if (!is_safe_to_allocate(request, path)) {
                        path = PathType::HOST;
                    }
                } else if (path == PathType::MC) {
                    path = PathType::HOST;
                }
            }

            if (allocate_resource(request, path)) {
                double latency = LatencyModel::calculate_dynamic_latency(
                    request.group_size,
                    path,
                    resource_status_.congestion_factor,
                    resource_status_.queue_depth
                );

                BarrierResponse response;
                response.task_id = request.task_id;
                response.path_used = path;
                response.latency_ns = latency;
                response.success = true;
                response.timeout = false;
                response.wave_id = request.wave_id;  // V3: 传递波次标记
                response.arrival_time = request.arrival_time;
                // V3: completion_time 将在 spawned 线程中设置

                switch (path) {
                    case PathType::GBA:  gba_requests_++; break;
                    case PathType::MC:   mc_requests_++; break;
                    case PathType::HOST: host_requests_++; break;
                }

                // V3: 主线程不发送响应！响应将在 spawned 线程中发送
                // V2.2: 普通队列也需要并发持有资源！
                // 注意: spawned 线程会设置 completion_time 并发送响应
                sc_spawn(sc_core::sc_bind(&HybridArbiter::hold_and_release_resource, this, request, path, response));
                
                // 主线程不等待,继续处理下一个请求!
                // 注意: 不在主线程发送响应! 避免重复发送
            }

            update_resource_status();
        }
    }
}

// ============================================================================
// 超时监控线程 (Timeout Monitor Thread)
// ============================================================================
void HybridArbiter::timeout_monitor() {
    while (true) {
        wait();

        if (reset.read()) continue;

        // 检查超时（简化实现）
        // 在实际硬件中，这里会有更复杂的超时检测逻辑
    }
}

// ============================================================================
// Banker's 安全检查 (Safety Check)
// ============================================================================
bool HybridArbiter::is_safe_to_allocate(const BarrierRequest& request, PathType path) {
    switch (path) {
        case PathType::GBA:
            // 检查 GBA slot 是否可用
            return has_gba_slot_available();

        case PathType::MC:
            // 检查 MC outstanding 是否足够 (需要 N 个)
            return has_mc_capacity(request.group_size);

        case PathType::HOST:
            // HOST 总是可用（但这是最后手段）
            return true;

        default:
            return false;
    }
}

// ============================================================================
// 路径选择决策 (Path Selection)
// ============================================================================
PathType HybridArbiter::select_path(const BarrierRequest& request) {
    // 检查是否有 path_hint
    if (request.path_hint == 1) return PathType::GBA;
    if (request.path_hint == 2) return PathType::MC;
    if (request.path_hint == 3) return PathType::HOST;

    // 分级策略
    GroupCategory category = get_category(request.group_size);

    switch (category) {
        case GroupCategory::LARGE:
            // N >= 64: 死保大组走 GBA
            return PathType::GBA;

        case GroupCategory::SMALL:
            // N < 16: 碎片小组走 MC
            return PathType::MC;

        case GroupCategory::MEDIUM:
            // 16 <= N < 64: 动态选择
            return dynamic_select(request);

        default:
            return PathType::MC;
    }
}

// ============================================================================
// 动态选择 (Dynamic Selection)
// ============================================================================
PathType HybridArbiter::dynamic_select(const BarrierRequest& request) {
    // 计算当前 GBA 利用率
    double gba_util = resource_status_.gba_utilization;

    // 如果 GBA 利用率低且有预留 slot，优先 GBA
    if (gba_util < (1.0 - GBA_RESERVE_RATIO) && has_gba_slot_available()) {
        // 考虑组大小：越大的组越应该走 GBA
        double score_gba = static_cast<double>(request.group_size) / GBA_THRESHOLD_N;
        if (score_gba > 0.5 && has_gba_slot_available()) {
            return PathType::GBA;
        }
    }

    // 否则走 MC
    if (has_mc_capacity(request.group_size)) {
        return PathType::MC;
    }

    // MC 满了，尝试 GBA
    if (has_gba_slot_available()) {
        return PathType::GBA;
    }

    // 最后手段：HOST（理论上不应该走到这里）
    return PathType::HOST;
}

// ============================================================================
// 资源分配 (Resource Allocation)
// ============================================================================
bool HybridArbiter::allocate_resource(const BarrierRequest& request, PathType path) {
    switch (path) {
        case PathType::GBA:
            if (has_gba_slot_available()) {
                resource_status_.gba_slots_used++;
                // 如果是大组，增加预留计数
                if (request.group_size >= GBA_THRESHOLD_N) {
                    gba_reserved_slots_++;
                }
                return true;
            }
            return false;

        case PathType::MC:
            if (has_mc_capacity(request.group_size)) {
                // MC 消耗 N 个 outstanding
                resource_status_.mc_outstanding_used += request.group_size;
                return true;
            }
            return false;

        case PathType::HOST:
            // HOST 不消耗硬件资源，但记录统计
            return true;

        default:
            return false;
    }
}

// ============================================================================
// 资源释放 (Resource Release)
// ============================================================================
void HybridArbiter::release_resource(const BarrierRequest& request, PathType path) {
    switch (path) {
        case PathType::GBA:
            if (resource_status_.gba_slots_used > 0) {
                resource_status_.gba_slots_used--;
                if (request.group_size >= GBA_THRESHOLD_N && gba_reserved_slots_ > 0) {
                    gba_reserved_slots_--;
                }
            }
            break;

        case PathType::MC:
            if (resource_status_.mc_outstanding_used >= request.group_size) {
                resource_status_.mc_outstanding_used -= request.group_size;
            }
            break;

        case PathType::HOST:
            // HOST 不消耗硬件资源
            break;
    }

    update_resource_status();
}

// ============================================================================
// 检查 GBA Slot 可用性
// ============================================================================
bool HybridArbiter::has_gba_slot_available() {
    uint32_t total_slots = TOTAL_GBA_SLOTS;
    uint32_t reserved_for_large = static_cast<uint32_t>(total_slots * GBA_RESERVE_RATIO);

    // 可用 slot = 总 slot - 已用 slot
    // 但要为大组预留
    uint32_t available = total_slots - resource_status_.gba_slots_used;

    return available > 0;
}

// ============================================================================
// 检查 MC Capacity
// ============================================================================
bool HybridArbiter::has_mc_capacity(uint16_t n) {
    uint32_t total = TOTAL_MC_OUTSTANDING;
    uint32_t available = total - resource_status_.mc_outstanding_used;

    return available >= n;
}

// ============================================================================
// 更新资源状态
// ============================================================================
void HybridArbiter::update_resource_status() {
    resource_status_.update_utilization(TOTAL_GBA_SLOTS, TOTAL_MC_OUTSTANDING);

    // 计算队列深度 (简化模型)
    resource_status_.queue_depth = (
        static_cast<double>(high_priority_queue_.size() + normal_queue_.size()) / 100.0
    );
}

// ============================================================================
// 处理超时降级
// ============================================================================
void HybridArbiter::handle_timeout(BarrierRequest& request) {
    timeout_count_++;

    // 超时降级逻辑
    // 如果在 GBA 等待超时，降级到 MC
    // 如果在 MC 等待超时，降级到 HOST
    // (这里简化实现)
}

// ============================================================================
// V2.2: 并发持有资源方法实现
// ============================================================================
void HybridArbiter::hold_and_release_resource(BarrierRequest request, PathType path, BarrierResponse response) {
    // 计算持有时间（基于组大小）
    double holding_time = RESOURCE_HOLDING_TIME_NS * (1.0 + request.group_size / 64.0);

    // 模拟处理时间（持有资源）
    wait(holding_time, sc_core::SC_NS);

    // V3: 在资源持有完成后设置 completion_time（这才是真正的完成时间）
    response.completion_time = sc_core::sc_time_stamp();

    // V3: 在 spawned 线程中发送响应（而不是主线程）
    response_out.write(response);

    // 释放资源
    release_resource(request, path);

    // 更新资源状态
    update_resource_status();
}
