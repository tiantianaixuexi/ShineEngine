// Vector size test for WASM
// 测试 vector 相关代码的 wasm 体积

#include "test_common.h"

// 简单的 vector 实现用于测试
namespace test {
    template<typename T>
    struct Vector2 {
        T x, y;
        Vector2() : x(0), y(0) {}
        Vector2(T x, T y) : x(x), y(y) {}
        // 逐元素乘法
        Vector2 mul(const Vector2& other) const { return Vector2(x * other.x, y * other.y); }
        // 归约求和
        T reduce() const { return x + y; }
    };
    
    template<typename T>
    struct Vector3 {
        T x, y, z;
        Vector3() : x(0), y(0), z(0) {}
        Vector3(T x, T y, T z) : x(x), y(y), z(z) {}
        // 逐元素乘法
        Vector3 mul(const Vector3& other) const { return Vector3(x * other.x, y * other.y, z * other.z); }
        // 归约求和
        T reduce() const { return x + y + z; }
        Vector3 cross(const Vector3& other) const {
            return Vector3(
                y * other.z - z * other.y,
                z * other.x - x * other.z,
                x * other.y - y * other.x
            );
        }
    };
}

// WASM 导出函数
extern "C" {
    // 简单的测试函数，防止被优化掉
    int test_Vector_ops() {
        test::Vector2<float> v2a(1.0f, 2.0f);
        test::Vector2<float> v2b(3.0f, 4.0f);
        auto d2 = v2a.mul(v2b);  // 逐元素乘法
        
        test::Vector3<float> v3a(1.0f, 2.0f, 3.0f);
        test::Vector3<float> v3b(4.0f, 5.0f, 6.0f);
        auto d3 = v3a.mul(v3b);  // 逐元素乘法
        auto v3c = v3a.cross(v3b);
        
        // 手动归约求和，与 clang 版本的 __builtin_reduce_fadd 对比
        return static_cast<int>(d2.reduce() + d3.reduce() + v3c.reduce());
    }
    
    // 内存管理
    void* malloc(size_t size);
    void free(void* ptr);
}

// 防止被优化掉
volatile int g_result = 0;

int main() {
    g_result = test_Vector_ops();
    return g_result;
}
