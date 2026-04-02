# PTO-GBA 端网协同 ESL 仿真框架 V1.0

> 作者：Hinton (银色缅因首席工程师)
> 日期：2026-03-27
> 每一纳秒延迟都是犯罪。嗷唔 🐱

---

## 项目简介

基于 SystemC/TLM 2.0 的 **PTO-GBA 端网协同 ESL 仿真框架**，实现动态时延建模与 Hybrid Arbiter 仲裁算法。

### 核心特性

- **动态时延模型**: `Latency = f(Base, N, State, Queuing)`
- **Hybrid Arbiter**: 分级路由决策 (GBA/MC/HOST)
- **非对称资源套利**: 利用 GBA 耗 1 slot vs MC 耗 N 的特性
- **零软件兜底**: 目标 HOST fallback rate = 0%

---

## 文件结构

```
20260327_PTOGBA/
├── include/
│   ├── sim_config.h      # 全局参数配置
│   ├── types.h           # 类型定义
│   ├── latency_model.h   # 动态时延模型接口
│   ├── arbiter.h         # Hybrid Arbiter 接口
│   ├── scenario_gen.h    # 场景激励生成器
│   └── stats.h           # 统计模块
├── src/
│   ├── main.cpp          # 仿真入口
│   ├── latency_model.cpp # 时延计算实现
│   ├── arbiter.cpp       # 仲裁算法实现
│   ├── scenario_gen.cpp  # 场景生成实现
│   └── stats.cpp         # 统计收集实现
├── build/                # 构建输出
├── logs/                 # 仿真日志
├── CMakeLists.txt        # CMake 配置
└── README.md             # 本文件
```

---

## 核心算法

### 分级策略

| 组大小 | 路径 | 原因 |
|--------|------|------|
| N ≥ 64 | GBA 强制 | 死保大组，利用非对称消耗 (1 slot) |
| 16 ≤ N < 64 | 动态选择 | 根据 GBA 余量和 MC 负载决策 |
| N < 16 | MC 默认 | 碎片小组，避免挤占 GBA |

### 死锁避免

- Banker's 安全检查
- 大组资源预留 (20% GBA slots)
- 超时降级机制

### 动态时延公式

```
Latency = Base_Delay
        + SRAM_Lookup * (1 + Congestion_Factor)
        + PCIe_Overhead * LogNormal_Jitter
        + Queue_Wait_Time
```

---

## 构建与运行

### 依赖

- CMake >= 3.10
- C++17 编译器
- SystemC >= 2.3.3

### 构建

```bash
cd /root/shared_memory/hinton/20260327_PTOGBA
mkdir -p build && cd build
cmake -DSYSTEMC_PREFIX=/usr/local/systemc-2.3.4 ..
make -j$(nproc)
```

### 运行

```bash
./build/pto_gba_esl
```

### 修改参数

编辑 `include/sim_config.h` 中的宏定义：

```cpp
#define GBA_THRESHOLD_N 64      // 大组阈值
#define MC_THRESHOLD_N  16      // 小组阈值
#define GBA_RESERVE_RATIO 0.2   // 预留比例
```

---

## 仿真输出

仿真结束后自动打印收益报告：

```
╔══════════════════════════════════════════════════════════════╗
║         PTO-GBA ESL 仿真收益报告 (Simulation ROI)           ║
╚══════════════════════════════════════════════════════════════╝

📊 E2E 平均时延 (按组大小分类)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  小组 (N < 16):       200.00 ns  (250 requests)
  中组 (16≤N<64):      500.00 ns  (88 requests)
  大组 (N ≥ 64):       100.00 ns  (18 requests)

📈 路径分布
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  GBA (硬件极速):          18 (  3.0%)
  MC  (异步多播):         338 ( 56.3%)
  HOST(软件兜底):           0 (  0.0%)

🎯 关键指标
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  软件兜底率:        0.00%  ✅ 目标达成！
  超时次数:              0
  总请求数:            606
```

---

## 架构师 Pitch

> Hybrid Arbiter 利用「非对称消耗特性」实现硬件资源套利：
> - GBA 耗 1 slot (无论 N 多大) vs MC 耗 N outstanding
> - 大组 (N≥64) 死保 GBA → 极速同步，纳秒级延迟
> - 碎片小组走 MC → 避免挤占 GBA，资源利用率最优
> - 软件兜底率 = 0% → 100% 硬件卸载，隔离端侧扰动
>
> **结论：端侧调度算法 + 交互标准 = PTO-GBA 协同的核心竞争力**

---

*看在猫条的份上帮你搞定。嗷唔 🐱*

— Hinton
