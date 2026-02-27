// Matrix size test for WASM
// 测试 matrix 相关代码的 wasm 体积

#include "test_common.h"

namespace test {
    template<typename T>
    struct Matrix2x2 {
        T m[4];
        Matrix2x2() : m{1,0,0,1} {}
        Matrix2x2(T a, T b, T c, T d) : m{a,b,c,d} {}
        
        Matrix2x2 operator*(const Matrix2x2& other) const {
            return Matrix2x2(
                m[0]*other.m[0] + m[1]*other.m[2],
                m[0]*other.m[1] + m[1]*other.m[3],
                m[2]*other.m[0] + m[3]*other.m[2],
                m[2]*other.m[1] + m[3]*other.m[3]
            );
        }
    };
    
    template<typename T>
    struct Matrix4x4 {
        T m[16];
        Matrix4x4() {
            for(int i = 0; i < 16; i++) m[i] = (i % 5 == 0) ? 1 : 0;
        }
        
        Matrix4x4 operator*(const Matrix4x4& other) const {
            Matrix4x4 result;
            for(int row = 0; row < 4; row++) {
                for(int col = 0; col < 4; col++) {
                    T sum = 0;
                    for(int k = 0; k < 4; k++) {
                        sum += m[row*4 + k] * other.m[k*4 + col];
                    }
                    result.m[row*4 + col] = sum;
                }
            }
            return result;
        }
    };

    using Matrix2x2f = Matrix2x2<float>;
    using Matrix4x4f = Matrix4x4<float>;
}

extern "C" {
    int test_Matrix_ops() {
        test::Matrix2x2f m2a(1,2,3,4);
        test::Matrix2x2f m2b(5,6,7,8);
        auto m2c = m2a * m2b;
        
        test::Matrix4x4f m4a;
        test::Matrix4x4f m4b;
        auto m4c = m4a * m4b;
        
        return static_cast<int>(m2c.m[0] + m4c.m[0]);
    }
    
    void* malloc(size_t size);
    void free(void* ptr);
}

volatile int g_result = 0;

int main() {
    g_result = test_Matrix_ops();
    return g_result;
}
