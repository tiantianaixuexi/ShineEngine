#pragma once

#include "math/vector.h"
#include "math/quat.h"
#include "math/vector2.h"




using FRotator3f = shine::math::TRotator<float>;
using FRotator3d = shine::math::TRotator<double>;
using FQuatf = shine::math::TQuat<float>;
using FQuatd = shine::math::TQuat<double>;


using FVector2 = shine::math::vector2<int>;
using FVector2f = shine::math::vector2<float>;
using FVector2d = shine::math::vector2<double>;
using FMatrix4f = shine::math::Matrix4<float>;
using FMatrix4d = shine::math::Matrix4<double>;
