"""
向量加法模块单元测试
Author: Hinton (Silver Maine Coon)
Date: 2026-03-30

TDD: 先写测试，再写实现。嗷唔。
"""

import pytest
from vector import Vector3D


class TestVector3DAddition:
    """向量加法测试类"""

    def test_basic_addition(self):
        """基本加法: (1, 2, 3) + (4, 5, 6) = (5, 7, 9)"""
        v1 = Vector3D(1, 2, 3)
        v2 = Vector3D(4, 5, 6)
        result = v1 + v2
        assert result.x == 5
        assert result.y == 7
        assert result.z == 9

    def test_zero_vector(self):
        """零向量: (1, 2, 3) + (0, 0, 0) = (1, 2, 3)"""
        v1 = Vector3D(1, 2, 3)
        zero = Vector3D(0, 0, 0)
        result = v1 + zero
        assert result.x == 1
        assert result.y == 2
        assert result.z == 3

    def test_negative_numbers(self):
        """负数: (1, 2, 3) + (-1, -2, -3) = (0, 0, 0)"""
        v1 = Vector3D(1, 2, 3)
        v2 = Vector3D(-1, -2, -3)
        result = v1 + v2
        assert result.x == 0
        assert result.y == 0
        assert result.z == 0

    def test_float_numbers(self):
        """浮点数: (0.5, 0.5, 0.5) + (0.5, 0.5, 0.5) = (1.0, 1.0, 1.0)"""
        v1 = Vector3D(0.5, 0.5, 0.5)
        v2 = Vector3D(0.5, 0.5, 0.5)
        result = v1 + v2
        assert result.x == pytest.approx(1.0)
        assert result.y == pytest.approx(1.0)
        assert result.z == pytest.approx(1.0)

    def test_repr(self):
        """字符串表示: Vector3D(1, 2, 3) 应该显示为 'Vector3D(1, 2, 3)'"""
        v = Vector3D(1, 2, 3)
        assert repr(v) == "Vector3D(1, 2, 3)"


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
