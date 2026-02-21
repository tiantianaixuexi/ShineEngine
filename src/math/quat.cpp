#include "quat.h"

#include "math/mathDef.h"

namespace shine::math
{

    template<FloatingPoint T>
    constexpr TQuat<T> TQuat<T>::fromRotatorDegrees(const TRotator<T>& rotDeg) noexcept
    {
        // 输入角度，将其转换为弧度按 (roll, pitch, yaw) 顺序生成
        const T k = T(0.017453292519943295769); // PI/180
        std::array<T,3> eulerRad{ rotDeg.Roll * k, rotDeg.Pitch * k, rotDeg.Yaw * k };
        return fromEulerAngles(eulerRad);
    }


    template<FloatingPoint T>
    constexpr TQuat<T> TQuat<T>::fromEulerAngles(const std::array<T, 3>& euler) noexcept
    {
        T cr = std::cos(euler[0] / T(2));
        T sr = std::sin(euler[0] / T(2));
        T cp = std::cos(euler[1] / T(2));
        T sp = std::sin(euler[1] / T(2));
        T cy = std::cos(euler[2] / T(2));
        T sy = std::sin(euler[2] / T(2));
        return TQuat(
            cr * cp * cy + sr * sp * sy,
            sr * cp * cy - cr * sp * sy,
            cr * sp * cy + sr * cp * sy,
            cr * cp * sy - sr * sp * cy
        );
    }

    template class TQuat<double>;
    template class TQuat<float>;
}

 
