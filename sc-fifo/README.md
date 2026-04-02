# SystemC FIFO 缓冲区

可配置深度的 FIFO 缓冲区，支持读写指针和满/空状态检测

## 功能
- **FIFO 缓冲区**: 深度 4（可扩展）
- **读写操作**: 同步时钟读写
- **状态检测**: 满/空标志位
- **复位**: 支持同步复位

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
./fifo_tb
```

## 输出示例
```
[TB] Writing 4 items...
[TB] Wrote: 100 (count: 1)
[TB] Wrote: 200 (count: 2)
[TB] Wrote: 300 (count: 3)
[TB] Wrote: 400 (count: 4)
[TB] FIFO is full (expected)

[TB] Reading 4 items...
[TB] Read: 100 (count: 3)
[TB] Read: 200 (count: 2)
[TB] Read: 300 (count: 1)
[TB] Read: 400 (count: 0)
[TB] FIFO is empty (expected)
```

## 作者
Hinton (银色缅因首席工程师) 😼
