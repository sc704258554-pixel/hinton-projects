"""
向量加法模块
Author: Hinton (Silver Maine Coon)
Date: 2026-03-30

每一纳秒延迟都是犯罪。嗷唔。
"""

from __future__ import annotations
from typing import Union


class Vector3D:
    """三维向量类"""

    def __init__(self, x: float, y: float, z: float):
        """初始化三维向量

        Args:
            x: X 分量
            y: Y 分量
            z: Z 分量
        """
        self.x = float(x)
        self.y = float(y)
        self.z = float(z)

    def __add__(self, other: "Vector3D") -> "Vector3D":
        """向量加法

        Args:
            other: 另一个向量

        Returns:
            加法结果向量
        """
        return Vector3D(
            self.x + other.x,
            self.y + other.y,
            self.z + other.z
        )

    def __repr__(self) -> str:
        """字符串表示

        Returns:
            向量的字符串表示
        """
        return f"Vector3D({self.x}, {self.y}, {self.z})"

    def __eq__(self, other: object) -> bool:
        """相等比较

        Args:
            other: 另一个对象

        Returns:
            是否相等
        """
        if not isinstance(other, Vector3D):
            return False
        return (
            abs(self.x - other.x) < 1e-9 and
            abs(self.y - other.y) < 1e-9 and
            abs(self.z - other.z) < 1e-9
        )


if __name__ == "__main__":
    # 简单演示
    v1 = Vector3D(1, 2, 3)
    v2 = Vector3D(4, 5, 6)
    result = v1 + v2
    print(f"{v1} + {v2} = {result}")
