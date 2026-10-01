#pragma once

#include <cmath>
#include <iostream>

namespace math
{

struct Vec3
{
    double x, y, z;

    double length() const { return std::sqrt(x * x + y * y + z * z); }

    Vec3 normalize() const
    {
        double len = length();
        return len > 0 ? Vec3{x / len, y / len, z / len} : Vec3{0, 0, 0};
    }

    Vec3 cross(const Vec3 &other) const
    {
        return {y * other.z - z * other.y, z * other.x - x * other.z, x * other.y - y * other.x};
    }
};

inline void print(const Vec3 &v)
{
    std::cout << std::format("Vec3({}, {}, {})", v.x, v.y, v.z) << '\n';
}

} // namespace math
