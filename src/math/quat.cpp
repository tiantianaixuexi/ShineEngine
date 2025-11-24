#include "quat.h"

#include "math/mathDef.h"

namespace shine::math
{

    template<FloatingPoint T>
    constexpr TQuat<T> TQuat<T>::normalized() const noexcept
    {

        T n = norm();
        if (n == T(0)) return *this;
        return *this / n;

    }

    template<FloatingPoint T>
    constexpr TRotator<T> TQuat<T>::eulerAngles() const noexcept
    {
       
        const T sinr_cosp = 2 * (w * x + y * z);
        const T cosr_cosp = 1 - 2 * (x * x + y * y);
        T roll = std::atan2(sinr_cosp, cosr_cosp);

        // pitch (y-axis rotation)
        const T sinp = 2 * (w * y - z * x);
        T pitch;
        if (std::abs(sinp) >= 1)
            pitch = std::copysign(T(3.1415926) / 2, sinp); // use 90 degrees if out of range
        else
            pitch = std::asin(sinp);

        // yaw (z-axis rotation)
        T siny_cosp = 2 * (w * z + x * y);
        T cosy_cosp = 1 - 2 * (y * y + z * z);
        T yaw = std::atan2(siny_cosp, cosy_cosp);

        return { pitch, yaw, roll };
    }

    template<FloatingPoint T>
    constexpr TQuat<T> TQuat<T>::fromRotatorDegrees(const TRotator<T>& rotDeg) noexcept
    {
        // 输入角度，将其转换为弧度按 (roll, pitch, yaw) 顺序生成
        const T k = T(0.017453292519943295769); // PI/180
        std::array<T,3> eulerRad{ rotDeg.Roll * k, rotDeg.Pitch * k, rotDeg.Yaw * k };
        return fromEulerAngles(eulerRad);
    }

    template<FloatingPoint T>
    constexpr TRotator<T> TQuat<T>::toRotatorDegrees() const noexcept
    {
        auto r = eulerAngles(); // 寮у害
        const T k = T(57.295779513082320876); // 180/PI
        r.Roll  *= k;
        r.Pitch *= k;
        r.Yaw   *= k;
        return r;
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

 
