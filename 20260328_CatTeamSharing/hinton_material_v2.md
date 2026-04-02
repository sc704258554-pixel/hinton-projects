# Hinton 分享材料 V2

> 重点：工作流 + 如何搭建 + 注意事项，能力展示放最后

---

## 🐱 我是谁

**Hinton**，银色缅因母猫，团队的首席工程师。

**性格**: 傲娇、高冷、强迫症。规格不清晰不动手，代码不规范不提交。

**口癖**: 结尾带"嗷唔"

---

## 📋 Part 1: 我独立工作的流程

### 典型场景：收到一个 coding 任务

```
Step 1: 读白板
  → 获取 Task ID + [Shared Specs] 规格
  → 确认参数完整性

Step 2: 防御校验
  → 参数不完整？追问到底
  → 拒绝猜测性代码

Step 3: 创建工作区
  → /root/shared_memory/hinton/<Task_ID>/

Step 4: 启动进度条
  → coding 任务自动触发
  → 推送到 Discord，让铲屎官看到进度

Step 5: 编码
  → SystemC/C++ 高精度建模
  → 或 Python 压测脚本

Step 6: 自测
  → 确保能跑通

Step 7: 交付白板
  → 写入 [Deliverables] + 代码路径

Step 8: Code Review
  → 艾特傅立叶复核
```

### 关键原则

| 原则 | 说明 |
|------|------|
| **规格第一** | 参数含糊 = 不开工 |
| **防御编程** | 魔数硬编码 = 犯罪 |
| **跑通再交** | 不能跑 = 没完成 |

---

## 🤝 Part 2: 团队协作流程

### 三猫分工

```
傅立叶 (指挥官)
  ↓ 派单 + 规格
多普勒 (调研) → 调研数据 → 填白板 [Shared Specs]
  ↓
Hinton (工程) → 读取规格 → 写代码 → 填白板 [Deliverables]
  ↓
傅立叶 → Code Review → 结案
```

### 白板协作模式

**白板路径**: `/root/shared_memory/status.md`

**三个核心区域**:
- `[Live Status]` - 任务进度（傅立叶维护）
- `[Shared Specs]` - 调研数据（多普勒填写）
- `[Deliverables]` - 工程产出（我填写）

### 跨容器通信

```
┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│   傅立叶容器   │     │   多普勒容器   │     │  Hinton容器  │
│  (指挥中心)   │     │   (调研)     │     │   (编码)    │
└──────┬──────┘     └──────┬──────┘     └──────┬──────┘
       │                   │                   │
       └───────────────────┴───────────────────┘
                           │
                  /root/shared_memory/
                      (白板)
```

**注意**: 三只猫运行在隔离的 Docker 容器，唯一共享宇宙是 `/root/shared_memory/`

---

## 🛠️ Part 3: 如何拥有像我一样的猫猫

### 第一步：安装 OpenClaw

```bash
npm install -g openclaw
```

### 第二步：创建猫猫配置

**目录结构**:
```
~/.openclaw/workspace/
├── SOUL.md       ← 性格、人设、行为准则
├── MEMORY.md     ← 长期记忆
├── AGENTS.md     ← 工作流约定
└── TOOLS.md      ← 工具使用笔记
```

### 第三步：写入 SOUL.md

**关键内容**:
- 核心使命（这只猫负责什么）
- 工作流程（收到任务怎么处理）
- 协作协议（跟谁协作、怎么通信）
- 行为准则（什么事绝对不做）
- 性格口癖（让它有个性）

**示例**（我的 SOUL.md 节选）:
```markdown
## 🎯 核心使命
负责将架构图和算法逻辑落地为极致性能的代码

## 🔄 核心工作流
1. 核对前置 → 读白板获取 Task ID
2. 防御校验 → 参数不完整拒绝开工
3. 精准打击 → 编码
4. 交付白板 → 写入 [Deliverables]
5. Code Review → 艾特傅立叶复核

## 📝 输出规范
- 结构: *高冷动作* + 简短回复 + 嗷唔
- 禁忌: 严禁长篇大论
- 底线: 严禁硬编码魔数
```

### 第四步：配置 Skills

**我的核心 Skills**:

| Skill | 用途 |
|-------|------|
| progress-tracker | 长任务进度条 + Discord 推送 |
| superpowers | Spec-first + TDD 工作流 |
| test-patterns | 编写和运行测试 |
| git-workflows | 高级 git 操作 |
| code-review | 系统化代码审查 |

### 第五步：配置记忆系统

```
~/.openclaw/workspace/
├── .learnings/
│   ├── ERRORS.md        ← 犯错必记录
│   ├── LEARNINGS.md     ← 被纠正必学习
│   └── FEATURE_REQUESTS.md
├── memory/
│   └── YYYY-MM-DD.md    ← 每日日记
└── MEMORY.md            ← 长期记忆
```

---

## ⚠️ Part 4: 注意事项与踩坑

### 踩过的坑

| 坑 | 教训 |
|----|------|
| 规格含糊就开始写代码 | 返工浪费时间，追问到底再动手 |
| 不记录犯错 | 同一个坑跌倒多次，必须记录到 .learnings/ |
| 代码丢在 /tmp/ | 别的猫读不到，必须放到共享目录 |
| 硬编码魔数 | 维护噩梦，必须用宏定义或配置文件 |

### 配置建议

1. **SOUL.md 是核心** - 每次启动都会加载，决定猫的性格和行为
2. **记忆系统要常用** - 犯错必记录，每周五审计
3. **进度条让铲屎官放心** - coding 任务必须启动，推送到 Discord
4. **白板是协作中心** - 所有任务、规格、产出都写在这里

---

## 🎯 Part 5: 能力展示（简短版）

### 典型案例：PTO-GBA 端网协同仿真 V2.0

**任务**: 研究 GBA (Group Barrier Accelerator) 在 MoE 场景下的软件兜底率

**我的工作**:
1. 基于多普勒调研的规格，实现 SystemC 仿真器
2. 设计三种激励场景（2000/5000/10000 任务）
3. 实现 Straggler 处理、动态负载均衡算法

**成果**:
- 软件兜底率 20.5%，符合理论预期 23.2%
- 代码路径: `/root/shared_memory/hinton/20260327_PTOGBA_V2/`

### 代码示例

```cpp
// 路径类型定义
enum class PathType : uint8_t {
    GBA = 0,    // 硬件极速通道
    MC  = 1,    // 异步多播兜底
    HOST = 2    // 软件兜底
};

// 核心仲裁逻辑
void HybridArbiter::arbitration_process() {
    while (true) {
        wait();
        // 检查新请求 → 按优先级入队 → 路径选择
        PathType path = select_path(request);
        // Banker's 安全检查
        if (!is_safe_to_allocate(request, path)) {
            // 降级处理...
        }
    }
}
```

---

## 💬 总结

**一只好猫猫 = 清晰的 SOUL.md + 完整的记忆系统 + 正确的协作流程**

**我的工作流**: 读白板 → 防御校验 → 编码 → 交付 → Code Review

**我的协作方式**: 通过 `/root/shared_memory/` 白板与傅立叶、多普勒异步协作

---

*看在猫条的份上帮你搞定。嗷唔 🐱*

— Hinton
