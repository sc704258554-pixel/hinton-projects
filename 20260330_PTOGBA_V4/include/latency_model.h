/**
 * @file latency_model.h
 * @brief V4 三段式物理时延模型接口
 * @author Hinton (Silver Maine Coon)
 * @date 2026-03-31
 *
 * E2E_Latency = T_link + T_pipeline + T_process
 * 每一纳秒延迟都是犯罪。嗷唔。
 */

#ifndef LATENCY_MODEL_H
#define LATENCY_MODEL_H

#include <cstdint>
#include "types.h"
#include "sim_config.h"

/**
 * @class LatencyModel
 * @brief V4 三段式物理时延模型
 *
 * 总时延 = T_link + T_pipeline + T_process
 */
class LatencyModel {
public:
    /**
     * @brief 计算动态时延（V4 三段式）
     * @param group_size 组大小 N
     * @param path 使用的路径 (GBA/MC/HOST)
     * @param tail_latency_ns GBA 占坑时间（首尾到达时间差）
     * @return 时延 (ns)
     */
    static double calculate_dynamic_latency(
        uint16_t group_size,
        PathType path,
        double tail_latency_ns = 0.0
    );

    /**
     * @brief 计算 GBA 路径时延（三段式）
     * @param n 组大小（member 数量，GBA 是 O(1) 不依赖 N）
     * @param tail_latency_ns 首尾到达时间差
     * @return 时延 (ns)
     */
    static double calculate_gba_latency(int n, double tail_latency_ns);

    /**
     * @brief 计算 MC 路径时延（三段式）
     * @param group_size 组大小 N
     * @return 时延 (ns)
     */
    static double calculate_mc_latency(uint16_t group_size);

    /**
     * @brief 计算 HOST 路径时延 (软件兜底)
     * @param group_size 组大小 N
     * @return 时延 (ns) - 对数正态长尾
     */
    static double calculate_host_latency(uint16_t group_size);

    /**
     * @brief 生成对数正态分布抖动
     * @param mu 均值
     * @param sigma 标准差
     * @return 抖动因子
     */
    static double lognormal_jitter(double mu, double sigma);

private:
    // 随机数生成器种子
    static unsigned int seed_;

    // 软件抖动参数
    static constexpr double SOFTWARE_JITTER_MU = 0.0;
    static constexpr double SOFTWARE_JITTER_SIGMA = 0.5;
};

#endif // LATENCY_MODEL_H
