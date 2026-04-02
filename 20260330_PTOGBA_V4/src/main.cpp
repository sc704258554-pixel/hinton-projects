/**
 * @file main.cpp
 * @brief V4 测试入口
 * @author Hinton (Silver Maine Coon)
 * @date 2026-03-31
 *
 * 测试三段式物理时延模型 + 激励生成器。
 * 每一纳秒延迟都是犯罪。嗷唔。
 */

#include <iostream>
#include <iomanip>
#include "sim_config.h"
#include "latency_model.h"
#include "stimulus_gen.h"

// ============================================================================
// 测试函数
// ============================================================================

void test_latency_model() {
    std::cout << "\n=== 三段式物理时延模型测试 ===\n\n";

    // 第一段 + 第二段
    std::cout << "第一段 T_link = " << T_LINK_NS << " ns\n";
    std::cout << "第二段 T_pipeline = " << T_PIPELINE_NS << " ns\n\n";

    // GBA 时延（不同 tail）
    std::cout << "GBA 时延测试:\n";
    std::cout << "  tail = 0 ns:    E2E = " << calculate_e2e_latency_gba(0, 0.0) << " ns\n";
    std::cout << "  tail = 100 ns:  E2E = " << calculate_e2e_latency_gba(0, 100.0) << " ns\n";
    std::cout << "  tail = 1000 ns: E2E = " << calculate_e2e_latency_gba(0, 1000.0) << " ns\n";
    std::cout << "  tail = 5000 ns: E2E = " << calculate_e2e_latency_gba(0, 5000.0) << " ns\n\n";

    // MC 时延（不同 N）
    std::cout << "MC 时延测试:\n";
    for (int n : {8, 16, 32, 64, 128}) {
        std::cout << "  N = " << std::setw(3) << n << ": E2E = "
                  << calculate_e2e_latency_mc(n) << " ns\n";
    }
    std::cout << "\n";
}

void test_scenario_a() {
    std::cout << "\n=== 场景 A 测试 (Mixtral 8x7B Layer 16) ===\n\n";

    auto requests = StimulusGenerator::generate_scenario_a();

    std::cout << "生成 " << requests.size() << " 个任务:\n";
    for (const auto& req : requests) {
        std::cout << "  Task " << req.task_id << ": N = " << req.group_size << "\n";
    }

    // 预期路径分布
    auto expected = StimulusGenerator::get_expected_path_distribution(ScenarioType::A_MIXTRAL_L16);
    std::cout << "\n预期路径分布:\n";
    std::cout << "  GBA:  " << expected[PathType::GBA] << " 任务\n";
    std::cout << "  MC:   " << expected[PathType::MC] << " 任务\n";
    std::cout << "  HOST: " << expected[PathType::HOST] << " 任务\n";
    std::cout << "  预期 HOST 率: 0%\n";
}

void test_scenario_b() {
    std::cout << "\n=== 场景 B 测试 (DeepSeek-V3 Layer 30) ===\n\n";

    auto requests = StimulusGenerator::generate_scenario_b();

    std::cout << "生成 " << requests.size() << " 个任务\n";

    // 统计 N 分布
    std::map<int, int> n_distribution;
    for (const auto& req : requests) {
        n_distribution[req.group_size]++;
    }

    std::cout << "\nN 分布统计:\n";
    std::cout << "  N 范围: 5-100\n";
    std::cout << "  N >= 64: " << n_distribution[64] << " 任务\n";
    std::cout << "  32 <= N < 64: " << n_distribution[32] << " 任务\n";

    // 预期路径分布
    auto expected = StimulusGenerator::get_expected_path_distribution(ScenarioType::B_DEEPSEEK_L30);
    std::cout << "\n预期路径分布:\n";
    std::cout << "  GBA:  " << expected[PathType::GBA] << " 任务\n";
    std::cout << "  MC:   " << expected[PathType::MC] << " 任务\n";
    std::cout << "  HOST: " << expected[PathType::HOST] << " 任务\n";
    std::cout << "  预期 HOST 率: 12.5%\n";
}

void test_scenario_c() {
    std::cout << "\n=== 场景 C 测试 (DeepSeek-V3 Pipeline 4 层) ===\n\n";

    auto requests = StimulusGenerator::generate_scenario_c();

    std::cout << "生成 " << requests.size() << " 个任务\n";

    // 统计每层任务数
    std::map<int, int> layer_tasks;
    for (const auto& [time, req] : requests) {
        layer_tasks[req.wave_id]++;
    }

    std::cout << "\n每层任务数:\n";
    for (const auto& [layer, count] : layer_tasks) {
        std::cout << "  Layer " << (layer + 1) << ": " << count << " 任务\n";
    }

    // 预期路径分布
    auto expected = StimulusGenerator::get_expected_path_distribution(ScenarioType::C_DEEPSEEK_PIPELINE);
    std::cout << "\n预期路径分布:\n";
    std::cout << "  GBA:  " << expected[PathType::GBA] << " 任务\n";
    std::cout << "  MC:   " << expected[PathType::MC] << " 任务\n";
    std::cout << "  HOST: " << expected[PathType::HOST] << " 任务\n";
    std::cout << "  预期 HOST 率: 18.75%\n";
}

// ============================================================================
// 主函数
// ============================================================================

int main(int argc, char* argv[]) {
    std::cout << "╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "║     PTO-GBA V4 - 三段式物理时延模型测试                  ║\n";
    std::cout << "║     Hinton (Silver Maine Coon)                           ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════╝\n";

    // 测试时延模型
    test_latency_model();

    // 测试激励场景
    test_scenario_a();
    test_scenario_b();
    test_scenario_c();

    std::cout << "\n=== 测试完成 ===\n";
    std::cout << "看在猫条的份上帮你搞定。嗷唔 🐱\n";

    return 0;
}
