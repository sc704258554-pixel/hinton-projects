# PTO-GBA V4 参数化改造

**Task ID**: `20260330_PTOGBA_V4`
**作者**: Hinton (银色缅因·首席工程师)
**日期**: 2026-03-31

---

## 📋 项目概述

V4 核心目标：
1. **参数化配置**: 三段式物理时延模型统一收拢到 `sim_config.h`
2. **激励准确模拟**: 待完成
3. **端到端收益对比**: 待完成

---

## ⚡ 三段式物理时延模型

**公式**: `E2E_Latency = T_link + T_pipeline + T_process`

| 段 | 参数 | 默认值 | 说明 |
|---|------|--------|------|
| **第一段** | `T_link` | 20 ns | 物理链路 RTT（2米光纤） |
| **第二段** | `T_pipeline` | 500 ns | 交换机流水线底噪 |
| **第三段** | `T_process` | 动态 | 核心处理与占坑时延 |

**T_process 详细定义**:

| 路径 | 处理时延 | 占坑逻辑 |
|------|---------|---------|
| **GBA** | O(1) ≈ 10 ns | 首尾到达时间差 |
| **MC** | O(N) = 50 ns + N×10 ns | 无需等待 |

---

## 📁 文件结构

```
/root/shared_memory/hinton/20260330_PTOGBA_V4/
├── include/
│   ├── sim_config.h      # V4 三段式时延配置
│   ├── types.h           # 基础类型定义
│   └── latency_model.h   # 时延模型接口
├── src/
│   └── latency_model.cpp # 时延模型实现
├── tests/
├── build/
├── logs/
└── README.md             # 本文件
```

---

## 🔧 使用方法

### 包含头文件

```cpp
#include "sim_config.h"
#include "latency_model.h"
```

### 计算 GBA 时延

```cpp
double tail_latency_ns = 1000.0;  // 首尾到达时间差
double e2e_gba = calculate_e2e_latency_gba(N, tail_latency_ns);
// = 20 + 500 + 10 + 1000 = 1530 ns
```

### 计算 MC 时延

```cpp
double e2e_mc = calculate_e2e_latency_mc(N);
// = 20 + 500 + 50 + N*10 ns
```

---

## 📚 参考资料

- **铲屎官澄清**: 三段式物理时延模型（2026-03-31）
- **调研报告**: `/root/shared_memory/doppler/20260330_PTOGBA_V4/latency_params_corrected.md`

---

*看在猫条的份上帮你搞定。嗷唔 🐱*

— Hinton
