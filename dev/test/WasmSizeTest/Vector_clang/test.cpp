// Vector size test for WASM
// 测试 vector 相关代码的 wasm 体积

#include "test_common.h"

// 简单的 vector 实现用于测试
namespace test {

    typedef float float2 __attribute__((ext_vector_type(2)));
    typedef float float3 __attribute__((ext_vector_type(3)));


    float3 cross(const float3& a, const float3& b){
        return {
            (a.y * b.z) - (a.z * b.y),
            (a.z * b.x) - (a.x * b.z),
            (a.x * b.y) - (a.y * b.x)
        };
    }
}

// WASM 导出函数
extern "C" {
    // 简单的测试函数，防止被优化掉
    int test_Vector_clang_ops() {
        test::float2 v2a{1.0f, 2.0f};
        test::float2 v2b{3.0f, 4.0f};
        test::float2 d2 = v2a * v2b;
        
        test::float3 v3a  {1.0f, 2.0f, 3.0f};
        test::float3 v3b  {4.0f, 5.0f, 6.0f};
        test::float3 d3 = v3a * v3b;
        test::float3 v3c = test::cross(v3a, v3b);
        
        // 使用 builtin_reduce_fadd 做浮点向量归约，比手动加更高效
        return static_cast<int>(
            (d2.x + d2.y) + 
            (d3.x + d3.y + d3.z) + 
            (v3c.x + v3c.y + v3c.z)
        );
    }
    
    // // 内存管理
    // void* malloc(size_t size);
    // void free(void* ptr);
}

// 防止被优化掉
volatile float g_result = 0;

int main() {
    auto result = test_Vector_clang_ops();
    return result;
}
