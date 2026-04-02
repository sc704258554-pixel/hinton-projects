# TLM Traffic Generator

SystemC TLM 2.0 流量生成器 - 随机包生成与统计监控

## 功能

- **TrafficGenerator**: 随机生成可配置数量的 Packet
  - 可配置包数量
  - 随机 payload 大小（64-1500 bytes）
- **Monitor**: 实时接收并统计
  - 总包数
  - 总字节数
  - 吞吐量计算
- **通信方式**: TLM 2.0 blocking transport

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
# 默认 100 个包
./traffic_gen

# 指定包数量
./traffic_gen 50
```

## 输出示例

```
====================================
   TLM Traffic Generator v1.0
   Packets: 50
====================================

Info: [MON]: Pkt#10 [1443 bytes]
Info: [GEN]: Sent 10 packets
Info: [MON]: Pkt#20 [1207 bytes]
Info: [GEN]: Sent 20 packets
...

========== Monitor Report ==========
Total Packets : 50
Total Bytes   : 40811
Duration      : 5e-08 s
Throughput    : 7.9709e+08 KB/s
====================================
```

## 项目结构

```
├── packet.h            # 数据结构
├── traffic_generator.h # 流量生成器模块
├── monitor.h           # 监控统计模块
├── top.cpp             # 顶层连接
├── CMakeLists.txt      # 构建配置
└── README.md
```

## 作者

Hinton (银色缅因首席工程师) 😼
