#ifndef VEC3_H
#define VEC3_H

#include "utils.h"

struct vec3 {
    float x, y, z;

    vec3(float scalar = 0) : x(scalar), y(scalar), z(scalar) {}
    vec3(float x, float y, float z) : x(x), y(y), z(z) {}
    vec3(const vec3& other) : x(other.x), y(other.y), z(other.z) {}

    // 在类里面实现, 默认inline

    vec3& operator+=(const vec3& other) {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    vec3& operator-=(const vec3& other) {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }

    vec3& operator*=(float scalar) {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        return *this;
    }

    bool operator==(const vec3& other) const {
        return nearlyEqual(x, other.x) &&
               nearlyEqual(y, other.y) &&
               nearlyEqual(z, other.z);
    }

    // =========================================
    // debug
    // void print() const {
    //     cout << "(" << x << ", " << y << ", " << z << ")" << endl;
    // }
};

inline vec3 operator+(const vec3& l, const vec3& r) {
    return {l.x + r.x, l.y + r.y, l.z + r.z};
}

inline vec3 operator-(const vec3& l, const vec3& r) {
    return {l.x - r.x, l.y - r.y, l.z - r.z};
}

// 取负
inline vec3 operator-(const vec3& v) {
    return {-v.x, -v.y, -v.z};
}

inline vec3 operator*(const vec3& v, float scalar) {
    return {v.x * scalar, v.y * scalar, v.z * scalar};
}

// 要支持左乘（float * vec3），必须使用非成员函数重载, 注意：要加 friend 或放到类外面
inline vec3 operator*(float scalar, const vec3& v) {
    return v * scalar;
}

// 两个vec对应坐标相乘得到第三个vec, 好像没有几何意义, 用的不多, 略.
// 除法略.

inline float dot(const vec3& l, const vec3& r) {
    return l.x * r.x + l.y * r.y + l.z * r.z;
}

inline float length2(const vec3& v) { return dot(v, v); }
inline float length(const vec3& v) { return sqrt(length2(v)); }
inline float distance(const vec3& p0, const vec3& p1) { return length(p1 - p0); }

inline vec3 normalize(const vec3& v) {
    float len = length(v);

    // 防止除0, 考虑到了就行, 不用太纠结.
    // glm的实现是
    //   如果v是全0, 结果undefined
    //   如果不是, e.g. vec3(0, 0, 0.0000001f), 返回(0,0,1)
    // 这里的实现是
    //   len接近0, 返回v
    if (nearlyEqual(len, 0.0f)) {
        return v;
    }

    return {v.x / len, v.y / len, v.z / len};
}

vec3 cross(const vec3& l, const vec3& r) {
    // 不要求记住这个公式
    return {
        l.y * r.z - l.z * r.y,
        l.z * r.x - l.x * r.z,
        l.x * r.y - l.y * r.x,
    };
}

vec3 reflect(const vec3& i, const vec3& n) {
    return i - 2 * dot(i, n) * n;
}

#endif  // VEC3_H