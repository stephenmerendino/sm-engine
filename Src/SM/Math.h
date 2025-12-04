#pragma once

#include "SM/StandardTypes.h"
#include <cfloat>
#include <cmath>
#include <ctime>

namespace SM
{
    class Vec2
    {
        public:
            Vec2();
    };

    class Vec3
    {
        public:
            Vec3();
    };

    class Vec4
    {
        public:
            Vec4();
    };

    class IVec2
    {
        public:
            IVec2();
    };

    class IVec3
    {
        public:
            IVec3();
    };

    class IVec4
    {
        public:
            IVec4();
    };

    class Mat33
    {
        public:
            Mat33();
    };

    class Mat43
    {
        public:
            Mat43();
    };

    class Mat44
    {
        public:
            Mat44();
    };

    constexpr F32 kPi = 3.1415926535897932384626433832795f;
    constexpr F32 k2Pi = 6.283185307179586476925286766559f;

    bool CloseEnough(F32 a, F32 b, F32 acceptableError = FLT_EPSILON);
    bool IsCloseEnoughToZero(F32 value, F32 acceptableError = FLT_EPSILON);

    inline F32 DegToRad(F32 degrees)
    {
        static const F32 DEGREES_CONVERSION = kPi / 180.0f;
        return degrees * DEGREES_CONVERSION;
    }

    inline F32 RadToDeg(F32 radians)
    {
        static const F32 RADIANS_CONVERSION = 180.0f / kPi;
        return radians * RADIANS_CONVERSION;
    }

    inline F32 SinDeg(F32 deg)
    {
        return sinf(DegToRad(deg));
    }

    inline F32 CosDeg(F32 deg)
    {
        return cosf(DegToRad(deg));
    }

    inline F32 TanDeg(F32 deg)
    {
        return tanf(DegToRad(deg));
    }

    inline void SeedRng()
    {
        ::srand((U32)time(NULL));
    }

    inline F32 RandomNumZeroToOne()
    {
        I32 num = ::rand();
        return (F32)num / (F32)RAND_MAX;
    }

    inline F32 RandomNumBetween(F32 low, F32 high)
    {
        F32 range = high - low;
        return low + (RandomNumZeroToOne() * range);
    }

    inline I32 RandomNumBetween(I32 low, I32 high)
    {
        I32 range = high - low;
        return low + (::rand() % range);
    }

    template<typename T>
    inline T Min(T a, T b)
    {
        return (a < b) ? a : b;
    }

    template<typename T>
    inline T Max(T a, T b)
    {
        return (a > b) ? a : b;
    }

    template<typename T>
    inline T Clamp(T value, T min, T max)
    {
        if(value < min) return min;
        if(value > max) return max;
        return value;
    }
}
