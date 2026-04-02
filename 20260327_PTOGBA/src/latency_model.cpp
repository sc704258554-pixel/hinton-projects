/**
 * @file latency_model.cpp
 * @brief 动态时延模型实现
 * @author Hinton (Silver Maine Coon)
 * @date 2026-03-27
 *
 * 每一纳秒延迟都是犯罪。嗷唔。
 */

#include "latency_model.h"
#include <cmath>
#include <cstdlib>
#include <iostream>

unsigned int LatencyModel::seed_ = RANDOM_SEED;

double LatencyModel::calculate_dynamic_latency(
    uint16_t group_size,
    PathType path,
    double congestion_factor,
    double queue_depth
) {
    switch (path) {
        case PathType::GBA:
            return calculate_gba_latency(congestion_factor);
        case PathType::MC:
            return calculate_mc_latency(group_size, congestion_factor, queue_depth);
        case PathType::HOST:
            return calculate_host_latency(group_size);
        default:
            return HOST_BASE_DELAY_NS * 10;  // 未知路径，惩罚
    }
}

double LatencyModel::calculate_gba_latency(double congestion_factor) {
    // GBA: 纯硬件汇聚，与 N 无关（非对称消耗特性！）
    // Latency = Base + SRAM_Lookup * (1 + congestion)

    double base = GBA_BASE_DELAY_NS;
    double sram = sram_lookup_delay(congestion_factor);

    // PCIe 开销 (发送端)
    double pcie = PCIE_OVERHEAD_NS * lognormal_jitter(SOFTWARE_JITTER_MU, SOFTWARE_JITTER_SIGMA);

    // 网络传输
    double network = NETWORK_RTT_NS;

    double total = base + sram + pcie + network;

    return total;
}

double LatencyModel::calculate_mc_latency(
    uint16_t group_size,
    double congestion_factor,
    double queue_depth
) {
    // MC: 微码复制，消耗 = N * Outstanding
    // 延迟与组大小正相关（但比 HOST 好太多）

    double base = MC_BASE_DELAY_NS;
    double microcode = MICROCODE_DELAY_NS * (1.0 + std::log2(group_size + 1) / 8.0);
    double sram = sram_lookup_delay(congestion_factor);
    double congestion = congestion_penalty(congestion_factor, queue_depth);

    // PCIe 开销
    double pcie = PCIE_OVERHEAD_NS * lognormal_jitter(SOFTWARE_JITTER_MU, SOFTWARE_JITTER_SIGMA);

    // 网络传输
    double network = NETWORK_RTT_NS;

    double total = base + microcode + sram + congestion + pcie + network;

    return total;
}

double LatencyModel::calculate_host_latency(uint16_t group_size) {
    // HOST: 软件兜底，对数正态长尾（地狱通道）
    // 这是你绝对不想走的路

    double base = HOST_BASE_DELAY_NS;

    // 组大小影响 (线性增长)
    double size_factor = group_size * 10.0;  // 每个成员 10ns 额外开销

    // 软件抖动 (对数正态长尾)
    double jitter = lognormal_jitter(SOFTWARE_JITTER_MU, SOFTWARE_JITTER_SIGMA);
    double jitter_penalty = base * jitter;  // 抖动惩罚

    double total = base + size_factor + jitter_penalty;

    return total;
}

double LatencyModel::lognormal_jitter(double mu, double sigma) {
    // Box-Muller 变换生成正态分布
    double u1 = static_cast<double>(rand_r(&seed_)) / RAND_MAX;
    double u2 = static_cast<double>(rand_r(&seed_)) / RAND_MAX;

    // 避免 log(0)
    if (u1 < 1e-10) u1 = 1e-10;

    double z = std::sqrt(-2.0 * std::log(u1)) * std::cos(2.0 * M_PI * u2);

    // 转换为对数正态
    double lognormal = std::exp(mu + sigma * z);

    return lognormal;
}

double LatencyModel::congestion_penalty(double congestion_factor, double queue_depth) {
    // 拥塞惩罚：当 queue_depth > 阈值时指数增长

    if (queue_depth <= BACKPRESSURE_THRESHOLD) {
        return 0.0;
    }

    // 指数惩罚
    double excess = queue_depth - BACKPRESSURE_THRESHOLD;
    double penalty = CONGESTION_PENALTY_FACTOR * 100.0 * std::exp(excess * 5.0);

    return penalty;
}

double LatencyModel::sram_lookup_delay(double congestion_factor) {
    // SRAM 查表时延：受拥塞影响
    return SRAM_LOOKUP_NS * (1.0 + congestion_factor);
}
