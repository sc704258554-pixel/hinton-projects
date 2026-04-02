/**
 * @file latency_model.cpp
 * @brief V4 三段式物理时延模型实现
 * @author Hinton (Silver Maine Coon)
 * @date 2026-03-31
 *
 * E2E_Latency = T_link + T_pipeline + T_process
 * 每一纳秒延迟都是犯罪。嗷唔。
 */

#include "latency_model.h"
#include <cmath>
#include <cstdlib>

unsigned int LatencyModel::seed_ = 42;

double LatencyModel::calculate_dynamic_latency(
    uint16_t group_size,
    PathType path,
    double tail_latency_ns
) {
    switch (path) {
        case PathType::GBA:
            return calculate_gba_latency(group_size, tail_latency_ns);
        case PathType::MC:
            return calculate_mc_latency(group_size);
        case PathType::HOST:
            return calculate_host_latency(group_size);
        default:
            return calculate_host_latency(group_size) * 2.0;
    }
}

double LatencyModel::calculate_gba_latency(int n, double tail_latency_ns) {
    (void)n;  // GBA 是 O(1)，不依赖 N
    // V4 三段式：GBA 路径
    // E2E = T_link + T_pipeline + T_process
    // T_process = T_GBA_PURE + tail_latency

    double T_process = T_GBA_PURE_NS + tail_latency_ns;
    double total = T_LINK_NS + T_PIPELINE_NS + T_process;

    return total;
}

double LatencyModel::calculate_mc_latency(uint16_t group_size) {
    // V4 三段式：MC 路径
    // E2E = T_link + T_pipeline + T_process
    // T_process = T_MC_BASE + N × T_MC_COPY_PER_MEMBER

    double T_process = T_MC_BASE_NS + group_size * T_MC_COPY_PER_MEMBER_NS;
    double total = T_LINK_NS + T_PIPELINE_NS + T_process;

    return total;
}

double LatencyModel::calculate_host_latency(uint16_t group_size) {
    // HOST: 软件兜底，对数正态长尾（地狱通道）
    // 这是你绝对不想走的路

    double base = calculate_e2e_latency_host(group_size);

    // 软件抖动 (对数正态长尾)
    double jitter = lognormal_jitter(SOFTWARE_JITTER_MU, SOFTWARE_JITTER_SIGMA);
    double jitter_penalty = base * (jitter - 1.0);

    return base + jitter_penalty;
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
