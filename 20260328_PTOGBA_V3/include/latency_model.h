/**
 * @file latency_model.h
 * @brief 动态时延模型接口
 * @author Hinton (Silver Maine Coon)
 * @date 2026-03-27
 *
 * Latency = f(Base, Length/N, Current_State, Queuing)
 * 每一纳秒延迟都是犯罪。嗷唔。
 */

#ifndef LATENCY_MODEL_H
#define LATENCY_MODEL_H

#include <cstdint>
#include "types.h"
#include "sim_config.h"

/**
 * @class LatencyModel
 * @brief 动态时延计算模型
 *
 * 实现 Latency = Base + Processing + Congestion + Jitter
 */
class LatencyModel {
public:
    /**
     * @brief 计算动态时延
     * @param group_size 组大小 N
     * @param path 使用的路径 (GBA/MC/HOST)
     * @param congestion_factor 拥塞因子 [0, 1]
     * @param queue_depth 队列深度
     * @return 时延 (ns)
     */
    static double calculate_dynamic_latency(
        uint16_t group_size,
        PathType path,
        double congestion_factor,
        double queue_depth
    );

    /**
     * @brief 计算 GBA 路径时延
     * @param congestion_factor 拥塞因子
     * @return 时延 (ns)
     */
    static double calculate_gba_latency(double congestion_factor);

    /**
     * @brief 计算 MC 路径时延
     * @param group_size 组大小 N (MC 消耗与 N 相关)
     * @param congestion_factor 拥塞因子
     * @param queue_depth 队列深度
     * @return 时延 (ns)
     */
    static double calculate_mc_latency(
        uint16_t group_size,
        double congestion_factor,
        double queue_depth
    );

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

    /**
     * @brief 计算拥塞惩罚
     * @param congestion_factor 拥塞因子
     * @param queue_depth 队列深度
     * @return 惩罚时延 (ns)
     */
    static double congestion_penalty(double congestion_factor, double queue_depth);

    /**
     * @brief 计算 SRAM 查表时延
     * @param congestion_factor 拥塞因子
     * @return 时延 (ns)
     */
    static double sram_lookup_delay(double congestion_factor);

private:
    // 随机数生成器种子
    static unsigned int seed_;
};

#endif // LATENCY_MODEL_H
