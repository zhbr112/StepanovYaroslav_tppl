import json
from plib.base import Point
import pytest


class TestPoint:
    def test_init(self):
        p = Point(1, 2)
        assert p.x == 1
        assert p.y == 2
        with pytest.raises(TypeError):
            Point(1.1, 2)

    def test_add(self):
        p1 = Point(1, 2)
        p2 = Point(3, 4)
        p3 = p1 + p2
        assert p3.x == 4
        assert p3.y == 6

    def test_iadd(self):
        p1 = Point(1, 2)
        p2 = Point(3, 4)
        p1 += p2
        assert p1.x == 4
        assert p1.y == 6

    def test_eq(self):
        p1 = Point(1, 2)
        p2 = Point(1, 2)
        p3 = Point(3, 4)
        assert p1 == p2
        assert p1 != p3

        with pytest.raises(NotImplementedError):
            p1 == 1

    def test_sub(self):
        p1 = Point(1, 2)
        p2 = Point(3, 4)
        p3 = p1 - p2
        assert p3.x == -2
        assert p3.y == -2

    def test_neg(self):
        p1 = Point(1, 2)
        p2 = -p1
        assert p2.x == -1
        assert p2.y == -2

    def test_to(self):
        p1 = Point(1, 2)
        p2 = Point(4, 6)
        assert abs(p1.to(p2) - 5.0) < 1e-6

    def test_str(self):
        p = Point(1, 2)
        assert str(p) == "Point(1, 2)"

    def test_is_center(self):
        assert Point(0, 0).is_center()
        assert not Point(1, 2).is_center()

    def test_from_json(self):
        p = Point(5, 7)
        p_json = '{"x": 5, "y": 7}'
        p2 = Point.from_json(p_json)
        assert p == p2

    def test_repr(self):
        p = Point(1, 2)
        assert repr(p) == "Point(1, 2)"

    def test_to_json(self):
        p = Point(5, 7)
        p_json = '{"x": 5, "y": 7}'
        assert p.to_json() == p_json
