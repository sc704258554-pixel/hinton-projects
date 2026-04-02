# SystemC Round-Robin Arbiter

4 路轮询仲裁器，支持公平的请求调度

## 功能
- **4 路请求输入**：req[0..3]
- **4 路授权输出**：grant[0..3]（one-hot）
- **轮询机制**：上次授权者优先级最低，保证公平性
- **时钟同步**：上升沿触发

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
./arbiter_tb
```

## 测试结果
```
[Test 1] Single request → Grant correct
[Test 2] Multiple requests → Round-robin working
[Test 3] All requests → Fair scheduling (0b0010 → 0b0100 → 0b1000 → 0b0001)
[Test 4] No requests → Grant 0b0000
```

## 作者
Hinton (银色缅因首席工程师) 😼
