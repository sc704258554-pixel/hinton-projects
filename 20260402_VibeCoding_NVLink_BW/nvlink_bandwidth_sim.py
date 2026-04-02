#!/usr/bin/env python3
"""
NVLink 4.0 带宽压测模拟器
模拟 8-GPU 节点上的 NVLink 全互联拓扑，验证理论带宽利用率。
"""

from __future__ import annotations
import sys
from dataclasses import dataclass, field
from typing import Dict, List, Tuple
import random

# ── 物理常量 (NVLink 4.0 规格) ──
NVLINK_LINK_BW_GBPS = 100.0       # 单链路 100 GB/s
NVLINK_LINKS_PER_GPU = 12          # 每个 GPU 12 条 NVLink
TOTAL_GPU_BW = NVLINK_LINK_BW_GBPS * NVLINK_LINKS_PER_GPU  # 1200 GB/s
GPU_COUNT = 8

@dataclass
class TransferRecord:
    src: int
    dst: int
    size_gb: float
    latency_us: float
    realized_bw_gbps: float

class NVLinkSimulator:
    """模拟 NVLink 4.0 全互联拓扑的带宽压测"""

    def __init__(self, num_gpus: int = GPU_COUNT, seed: int = 42):
        self.num_gpus = num_gpus
        self.rng = random.Random(seed)
        self.records: List[TransferRecord] = []

        # 全互联拓扑: 每个 GPU 到其他 GPU 有 2 条 NVLink
        # (NVLink 4.0: 12 链路分 6 组，每组 2 条连接一个 peer)
        self.topology: Dict[Tuple[int, int], int] = {}
        for i in range(num_gpus):
            for j in range(num_gpus):
                if i != j:
                    self.topology[(i, j)] = 2  # 2 链路 per peer pair

    def transfer(self, src: int, dst: int, size_gb: float) -> TransferRecord:
        """执行一次 GPU 间传输，模拟带宽争用"""
        if src == dst:
            raise ValueError(f"src == dst ({src})")
        if not (0 <= src < self.num_gpus and 0 <= dst < self.num_gpus):
            raise ValueError(f"GPU index out of range: [{src}, {dst}]")

        # 该 peer 对的可用链路带宽
        links = self.topology.get((src, dst), 0)
        max_bw = links * NVLINK_LINK_BW_GBPS  # 200 GB/s per peer pair

        # 模拟带宽争用: 75%~98% 利用率 (NVLink 4.0 实测范围)
        utilization = self.rng.uniform(0.75, 0.98)
        realized_bw = max_bw * utilization

        # 计算 latency
        latency_us = (size_gb * 1e9) / (realized_bw * 1e9) * 1e6  # GB / (GB/s) -> s -> us

        record = TransferRecord(
            src=src, dst=dst, size_gb=size_gb,
            latency_us=latency_us, realized_bw_gbps=realized_bw
        )
        self.records.append(record)
        return record

    def all_to_all(self, size_gb: float) -> List[TransferRecord]:
        """All-to-All 全互联压测"""
        results = []
        for src in range(self.num_gpus):
            for dst in range(self.num_gpus):
                if src != dst:
                    results.append(self.transfer(src, dst, size_gb))
        return results

    def ring_bcast(self, root: int, size_gb: float) -> List[TransferRecord]:
        """环形广播: root -> 0 -> 1 -> ... -> N-1"""
        results = []
        order = list(range(self.num_gpus))
        order.remove(root)
        order = [root] + order
        for i in range(len(order) - 1):
            results.append(self.transfer(order[i], order[i + 1], size_gb))
        return results

    def report(self) -> str:
        """生成压测报告"""
        if not self.records:
            return "No transfers recorded."

        total_size = sum(r.size_gb for r in self.records)
        avg_bw = sum(r.realized_bw_gbps for r in self.records) / len(self.records)
        min_bw = min(r.realized_bw_gbps for r in self.records)
        max_bw = max(r.realized_bw_gbps for r in self.records)
        avg_latency = sum(r.latency_us for r in self.records) / len(self.records)

        # 按 src-dst 对聚合，找最忙链路
        pair_bw: Dict[Tuple[int, int], list] = {}
        for r in self.records:
            pair_bw.setdefault((r.src, r.dst), []).append(r.realized_bw_gbps)

        busiest = max(pair_bw.items(), key=lambda x: len(x[1]))

        lines = [
            "=" * 60,
            "  NVLink 4.0 带宽压测报告",
            "=" * 60,
            f"  GPU 数量:        {self.num_gpus}",
            f"  单链路带宽:      {NVLINK_LINK_BW_GBPS} GB/s",
            f"  Peer 对链路:     2 (200 GB/s per pair)",
            f"  单 GPU 总带宽:   {TOTAL_GPU_BW} GB/s",
            "-" * 60,
            f"  传输次数:        {len(self.records)}",
            f"  总数据量:        {total_size:.1f} GB",
            f"  平均带宽:        {avg_bw:.1f} GB/s",
            f"  最低带宽:        {min_bw:.1f} GB/s",
            f"  最高带宽:        {max_bw:.1f} GB/s",
            f"  平均延迟:        {avg_latency:.2f} μs",
            f"  最忙链路:        GPU {busiest[0][0]} → GPU {busiest[0][1]} ({len(busiest[1])} 次)",
            "=" * 60,
        ]
        return "\n".join(lines)


def main():
    sim = NVLinkSimulator(num_gpus=8, seed=42)

    # ── 场景 1: 点对点压测 ──
    print("\n📊 场景 1: 点对点压测 (GPU 0 → GPU 1, 10GB)")
    rec = sim.transfer(0, 1, 10.0)
    print(f"  延迟: {rec.latency_us:.2f} μs | 带宽: {rec.realized_bw_gbps:.1f} GB/s")

    # ── 场景 2: All-to-All 全互联 ──
    print("\n📊 场景 2: All-to-All 全互联 (1GB each)")
    sim.records.clear()
    sim.all_to_all(1.0)
    print(sim.report())

    # ── 场景 3: 环形广播 ──
    print("\n📊 场景 3: 环形广播 (GPU 0 广播 5GB)")
    sim.records.clear()
    sim.ring_bcast(0, 5.0)
    print(sim.report())


if __name__ == "__main__":
    main()
