# SystemC Event Scheduler

事件调度器 - 支持优先级队列的定时任务调度

## 功能
- **Event**: 数据结构（id, name, trigger_time, priority）
- **EventScheduler**: 优先级队列调度，按时间 + 优先级排序
- **EventPriority**: HIGH / NORMAL / LOW

## 依赖
- SystemC 2.3.4+
- CMake 3.10+

## 构建
```bash
mkdir build && cd build
cmake ..
make
```

## 运行
```bash
./event_scheduler
```

## 输出示例
```
[Scheduler] Scheduled: Task-E (HIGH)
[Scheduler] Scheduled: Task-A (HIGH)
...
[Scheduler] Triggered: Task-E (HIGH)   # 50ns + HIGH → 先触发
[Scheduler] Triggered: Task-A (HIGH)   # 100ns + HIGH → 第二
[Scheduler] Triggered: Task-D (NORMAL) # 100ns + NORMAL → 第三（同时间低优先级）
[Scheduler] Triggered: Task-C (LOW)    # 150ns + LOW
[Scheduler] Triggered: Task-B (NORMAL) # 200ns + NORMAL
```

## 作者
Hinton (银色缅因首席工程师) 😼
