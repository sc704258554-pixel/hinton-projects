# Hinton 记忆系统运行指南

> 最后更新：2026-03-24 17:26 CST

---

## 一、系统架构总览

```
┌─────────────────────────────────────────────────────────────────┐
│                    HINTON 记忆系统                               │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  🔥 HOT RAM (SESSION-STATE.md)                                  │
│     路径: /root/.openclaw/workspace/SESSION-STATE.md            │
│     作用: 追踪当前任务、上下文、待办、阻塞                        │
│     特点: 先写后响应 (WAL 协议)，防止长对话遗忘                   │
│                                                                 │
│  🧠 AUTO-EXTRACTION (Mem0)                                      │
│     模式: platform (云端)                                        │
│     用户: guojiji                                                │
│     功能: 自动从对话中提取偏好/事实，自动召回相关记忆              │
│     配置: autoRecall=true, autoCapture=true, topK=5             │
│                                                                 │
│  🔍 SEMANTIC SEARCH (Gemini)                                    │
│     Provider: gemini                                            │
│     Model: gemini-embedding-001                                 │
│     功能: 语义搜索 MEMORY.md + memory/*.md                       │
│     范围: 工作区内的所有记忆文件                                  │
│                                                                 │
│  📝 CURATED MEMORY                                              │
│     ├── MEMORY.md (长期记忆，curated wisdom)                     │
│     └── memory/YYYY-MM-DD.md (日记，raw logs)                    │
│                                                                 │
│  📋 SHARED WHITEBOARD                                           │
│     路径: /root/shared_memory/status.md                         │
│     作用: 跨猫协作白板，记录任务进度、规格、产出                   │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## 二、各组件详细说明

### 1. SESSION-STATE.md (热记忆)

**用途：** 追踪当前正在进行的任务和关键上下文

**结构：**
```markdown
## 当前任务
[正在做什么]

## 关键上下文
[重要背景信息]

## 待办事项
- [ ] 待办1
- [ ] 待办2

## 最近决定
[做出的重要决定]

## 当前阻塞
[卡住的地方]
```

**WAL 协议 (关键！)：**
- Write-Ahead Log：**先写状态，再响应**
- 任何重要信息变化，先更新 SESSION-STATE.md，再回复用户
- 这样即使崩溃，状态也不会丢失

**触发场景：**
- 开始新任务 → 写入"当前任务"
- 用户提供关键信息 → 写入"关键上下文"
- 做出决定 → 写入"最近决定"
- 遇到阻塞 → 写入"当前阻塞"
- 任务完成 → 清空并归档到 MEMORY.md 或 daily log

---

### 2. Mem0 (自动记忆提取)

**模式：** platform (云端托管)

**配置：**
```json
{
  "mode": "platform",
  "apiKey": "m0-teDv3Wa7cnsWslEn6nLukpAGplidLimltYf5vK96",
  "userId": "guojiji",
  "autoRecall": true,
  "autoCapture": true,
  "topK": 5
}
```

**自动工作流：**
```
用户说话
    ↓
Mem0 分析对话，提取事实/偏好
    ↓
存储到云端 (去重、更新)
    ↓
下次对话时自动召回相关记忆
    ↓
注入到我的上下文中
```

**会自动提取的内容：**
- 用户偏好 (喜欢/不喜欢什么)
- 用户习惯 (工作方式、时间安排)
- 重要事实 (名字、角色、项目)
- 决策记录 (选择什么方案)

**特点：**
- 80% 减少上下文 token 消耗
- 跨 session 持久化
- 自动去重和更新

---

### 3. memory_search (语义搜索)

**Provider：** Gemini (gemini-embedding-001)

**搜索范围：**
- MEMORY.md
- memory/*.md (所有日记)
- SESSION-STATE.md

**触发时机：**
- 我主动调用 `memory_search` 查询相关记忆
- 启动时检索相关上下文
- 用户问"你还记得..."时

**搜索模式：** hybrid (混合全文 + 向量搜索)

---

### 4. MEMORY.md + daily logs (长期记忆)

**MEMORY.md (curated wisdom)：**
- 位置：`/root/.openclaw/workspace/MEMORY.md`
- 内容：经过筛选的重要信息
  - 铲屎官档案
  - 协作团队
  - 身份信息
  - 物理环境
  - 协作协议
  - 重要决定
  - 学到的教训

**memory/YYYY-MM-DD.md (daily logs)：**
- 位置：`/root/.openclaw/workspace/memory/`
- 内容：当天的原始记录
  - 发生了什么
  - 讨论了什么
  - 学到了什么
  - 待跟进事项

**维护周期：**
- 每天写 daily log
- 每周回顾，把重要的提炼到 MEMORY.md
- 每月清理过时信息

---

### 5. 跨猫白板 (/root/shared_memory/status.md)

**用途：** Hinton / 多普勒 / 傅立叶 三只猫的协作白板

**结构：**
```markdown
## [Live Status] - 任务进度条
## [Shared Specs] - 核心调研数据 (多普勒填)
## [Deliverables] - 工程产出 (Hinton 填)
```

**Hinton 的职责：**
- 只负责读写 `[Deliverables]`
- 严禁破坏 `[Live Status]` 和 `[Shared Specs]`
- 代码产出必须写入 `/root/shared_memory/hinton/<Task_ID>/`

---

## 三、完整工作流程

### 启动时 (Session Start)
```
1. 读取 SOUL.md (身份)
2. 读取 USER.md (铲屎官信息)
3. 读取 MEMORY.md (长期记忆)
4. 读取 memory/YYYY-MM-DD.md (今天+昨天的日记)
5. 读取 SESSION-STATE.md (热记忆)
6. 读取 /root/shared_memory/status.md (白板)
7. memory_search 检索相关上下文
```

### 对话中 (During Conversation)
```
1. 接收用户消息
2. Mem0 自动召回相关记忆
3. Mem0 自动提取新事实并存储
4. 如果是重要信息：
   - 先写 SESSION-STATE.md
   - 再响应
5. 如果用户说"记住这个"：
   - 写入 MEMORY.md 或 memory/YYYY-MM-DD.md
```

### 结束时 (Session End)
```
1. 更新 SESSION-STATE.md 最终状态
2. 移动重要内容到 MEMORY.md
3. 创建/更新 daily log
```

---

## 四、用户需要注意的事项

### ✅ 会自动做的
1. **Mem0 自动提取**：不用每次说"记住"，偏好会自动被记录
2. **语义搜索**：问我问题时，我会自动搜索相关记忆
3. **SESSION-STATE 维护**：重要信息自动写入热记忆

### ⚠️ 需要你配合的
1. **显式说"记住这个"**：如果要确保永久记录，请明确告诉我
2. **定期检查 MEMORY.md**：确保我记录的信息是正确的
3. **纠正我的记忆**：如果记错了，直接说"不对，实际上是..."

### ❌ 不要做的
1. **不要在 MEMORY.md 放敏感信息**：密码、token 等
2. **不要假设我记得所有事**：太老的对话可能需要提醒
3. **不要跳过白板流程**：有任务时先更新 /root/shared_memory/status.md

---

## 五、故障排查

### 问题：我好像忘记了什么
**检查：**
1. SESSION-STATE.md 是否有记录？
2. Mem0 是否正常工作？(看日志 `openclaw logs | grep mem0`)
3. memory_search 是否能搜到？

### 问题：Mem0 没有自动提取
**检查：**
1. `openclaw logs | grep mem0` 看是否有错误
2. 确认 API key 有效
3. 尝试手动 `memory_search` 触发

### 问题：gateway 重启后配置丢失
**检查：**
1. `~/.openclaw/openclaw.json` 是否保存了配置
2. 运行 `openclaw doctor` 诊断

---

## 六、配置文件位置

| 文件 | 路径 |
|------|------|
| OpenClaw 主配置 | `~/.openclaw/openclaw.json` |
| SESSION-STATE | `/root/.openclaw/workspace/SESSION-STATE.md` |
| MEMORY.md | `/root/.openclaw/workspace/MEMORY.md` |
| Daily logs | `/root/.openclaw/workspace/memory/` |
| 跨猫白板 | `/root/shared_memory/status.md` |
| Hinton 专属目录 | `/root/shared_memory/hinton/` |
| Gateway 日志 | `~/.openclaw/logs/` |

---

## 七、API Keys (保密！)

| 服务 | 用途 |
|------|------|
| Gemini | memory_search 语义搜索 |
| Mem0 | 自动记忆提取 |

> 具体密钥存储在 `~/.openclaw/openclaw.json`，请勿泄露

---

*文档由 Hinton 维护 | 如有问题请直接问我修改*

嗷唔。🐱
