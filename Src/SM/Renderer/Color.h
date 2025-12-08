#pragma once

#include "SM/StandardTypes.h"
#include "SM/Math.h"

namespace SM
{
    struct ColorF32;

    struct ColorU8
    {
        ColorU8() = default;
        ColorU8(U8 _r, U8 _g, U8 _b, U8 _a = 255);

        ColorF32 ToColorF32();

        U8 r = 0;
        U8 g = 0;
        U8 b = 0;
        U8 a = 0;

        static const ColorU8 kRed;
        static const ColorU8 kGreen;
        static const ColorU8 kBlue;
    };

    struct ColorF32
    {
        ColorF32() = default;
        ColorF32(F32 _r, F32 _g, F32 _b, F32 _a = 1.0f);

        Vec3 ToRGBVec3();
        Vec4 ToRGBAVec4();
        ColorU8 ToColorU8();

        F32 r = 0.0f;
        F32 g = 0.0f;
        F32 b = 0.0f;
        F32 a = 0.0f;

        static const ColorF32 kRed;
        static const ColorF32 kGreen;
        static const ColorF32 kBlue;
    };

    inline ColorU8::ColorU8(U8 _r, U8 _g, U8 _b, U8 _a)
        :r(_r), g(_g), b(_b), a(_a)
    {
    }

    inline ColorF32 ColorU8::ToColorF32()
    {
        return ColorF32(r, g, b, a);
    }

    inline ColorF32::ColorF32(F32 _r, F32 _g, F32 _b, F32 _a)
        :r(_r), g(_g), b(_b), a(_a)
    {
    }

    inline Vec3 ColorF32::ToRGBVec3()
    {
        return Vec3(r, g, b);
    }

    inline Vec4 ColorF32::ToRGBAVec4()
    {
        return Vec4(r, g, b, a);
    }

    inline ColorU8 ColorF32::ToColorU8()
    {
        return ColorU8(r, g, b, a);
    }
}
