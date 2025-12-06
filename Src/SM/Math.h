#pragma once

#include "SM/Engine.h"
#include "SM/Assert.h"
#include "SM/StandardTypes.h"
#include <cfloat>
#include <cmath>
#include <cstring>
#include <ctime>

namespace SM
{
    //-------------------------------------------------------------------------
    // Vec2
    //-------------------------------------------------------------------------
    class Vec2
    {
        public:
        F32 x = 0.0f;
        F32 y = 0.0f;

        Vec2() = default;
        Vec2(F32 _x, F32 _y);

    	Vec2 operator*(F32 s) const;
    	Vec2 operator/(F32 s) const;
    	Vec2& operator*=(F32 s);
    	Vec2& operator/=(F32 s);
    	Vec2 operator+(const Vec2& other) const;
    	Vec2 operator-(const Vec2& other) const;
    	Vec2& operator+=(const Vec2& other);
    	Vec2&operator-=(const Vec2& other);
    	Vec2 operator-() const;
    	bool operator==(const Vec2& other) const;

        F32 CalcLength() const;
        F32 CalcLengthSq() const;
        void SetLength(F32 length);
        void Normalize();
        Vec2 GetNormalized() const;

        static const Vec2 ZERO;
    };

    //-------------------------------------------------------------------------
    // Vec3
    //-------------------------------------------------------------------------
    class Vec3
    {
        public:
        F32 x = 0.0f;
        F32 y = 0.0f;
        F32 z = 0.0f;

        Vec3() = default;
        Vec3(F32 _x, F32 _y, F32 _z);
        Vec3(const Vec2& _xy, F32 _z = 0.0f);

        Vec3 operator*(F32 s) const;
        Vec3 operator/(F32 s) const;
        Vec3& operator*=(F32 s);
        Vec3& operator/=(F32 s);
        Vec3 operator+(const Vec3& other) const;
        Vec3& operator+=(const Vec3& other);
        Vec3 operator-(const Vec3& other) const;
        Vec3& operator-=(const Vec3& other);
        Vec3 operator-() const;
        bool operator==(const Vec3& other);

        F32 CalcLength() const;
        F32 CalcLengthSq() const;
        void SetLength(F32 length);
        void Normalize();
        Vec3 GetNormalized() const;

        static const Vec3 ZERO;
        static const Vec3 X_AXIS;
        static const Vec3 Y_AXIS;
        static const Vec3 Z_AXIS;
        static const Vec3 WORLD_FORWARD;
        static const Vec3 WORLD_BACKWARD;
        static const Vec3 WORLD_UP;
        static const Vec3 WORLD_DOWN;
        static const Vec3 WORLD_LEFT;
        static const Vec3 WORLD_RIGHT;
    };

    //-------------------------------------------------------------------------
    // Vec4
    //-------------------------------------------------------------------------
    class Vec4
    {
        public:
        F32 x = 0.0f;
        F32 y = 0.0f;
        F32 z = 0.0f;
        F32 w = 0.0f;

        Vec4() = default;
        Vec4(F32 _x, F32 _y, F32 _z, F32 _w);
        Vec4(const Vec2& _xy, F32 _z = 0.0f, F32 _w = 0.0f);
        Vec4(const Vec3& _xyz, F32 _w = 0.0f);

        Vec4 operator*(F32 s) const;
        Vec4 operator/(F32 s) const;
        Vec4& operator*=(F32 s);
        Vec4& operator/=(F32 s);
        Vec4 operator+(const Vec4& other) const;
        Vec4 operator-(const Vec4& other) const;
        Vec4& operator+=(const Vec4& other);
        Vec4& operator-=(const Vec4& other);
        Vec4 operator-() const;
        bool operator==(const Vec4& other) const;

        F32 CalcLengthSq() const;
        F32 CalcLength();
        void SetLength(F32 length);
        void Normalize();
        Vec4 GetNormalized() const;
        Vec3 ToVec3() const;

        static const Vec4 ZERO;
    };

    //-------------------------------------------------------------------------
    // IVec2
    //-------------------------------------------------------------------------
    class IVec2
    {
        public:
        I32 x = 0;
        I32 y = 0;

        IVec2() = default;
        IVec2(I32 _x, I32 _y);

    	IVec2 operator*(I32 s) const;
    	IVec2 operator/(I32 s) const;
    	IVec2& operator*=(I32 s);
    	IVec2& operator/=(I32 s);
    	IVec2 operator+(const IVec2& other) const;
    	IVec2 operator-(const IVec2& other) const;
    	IVec2& operator+=(const IVec2& other);
    	IVec2& operator-=(const IVec2& other);
    	IVec2 operator-() const;
    	bool operator==(const IVec2& other) const;

        F32 CalcLength() const;
        I32 CalcLengthSq() const;

        static const IVec2 ZERO;
    };

    //-------------------------------------------------------------------------
    // IVec3
    //-------------------------------------------------------------------------
    class IVec3
    {
        public:
        I32 x = 0;
        I32 y = 0;
        I32 z = 0;

        IVec3() = default;
        IVec3(I32 _x, I32 _y, I32 _z);
        IVec3(const IVec2& _xy, I32 _z = 0);

        IVec3 operator*(I32 s) const;
        IVec3 operator/(I32 s) const;
        IVec3& operator*=(I32 s);
        IVec3& operator/=(I32 s);
        IVec3 operator+(const IVec3& other) const;
        IVec3& operator+=(const IVec3& other);
        IVec3 operator-(const IVec3& other) const;
        IVec3& operator-=(const IVec3& other);
        IVec3 operator-() const;
        bool operator==(const IVec3& other);

        F32 CalcLength() const;
        I32 CalcLengthSq() const;

        static const IVec3 ZERO;
    };

    //-------------------------------------------------------------------------
    // Mat33
    //-------------------------------------------------------------------------
    class Mat33
    {
        public:
        F32 ix = 1.0f; F32 iy = 0.0f; F32 iz = 0.0f;
        F32 jx = 0.0f; F32 jy = 1.0f; F32 jz = 0.0f;
        F32 kx = 0.0f; F32 ky = 0.0f; F32 kz = 1.0f;

        Mat33() = default;
        Mat33(F32 _ix, F32 _iy, F32 _iz,
              F32 _jx, F32 _jy, F32 _jz,
              F32 _kx, F32 _ky, F32 _kz);
        Mat33(const Vec3& i, const Vec3& j, const Vec3& k);
        Mat33(F32* data);

        F32* operator[](U32 row);
        const F32* operator[](U32 row) const;
        Mat33 operator*(F32 s) const;
        Mat33& operator*=(F32 s);
        Mat33 operator*(const Mat33& other) const;
        Mat33& operator*=(const Mat33& other);

        Vec3 GetIBasis() const;
        Vec3 GetJBasis() const;
        Vec3 GetKBasis() const;
        void SetIBasis(F32 _ix, F32 _iy, F32 _iz);
        void SetJBasis(F32 _jx, F32 _jy, F32 _jz);
        void SetKBasis(F32 _kx, F32 _ky, F32 _kz);
        void SetIBasis(const Vec3& i);
        void SetJBasis(const Vec3& j);
        void SetKBasis(const Vec3& k);

        void Transpose();
        Mat33 GetTransposed() const;
        F32 Determinant() const;
    };

    //-------------------------------------------------------------------------
    // Mat44
    //-------------------------------------------------------------------------
    class Mat44
    {
        public:
        F32 ix = 1.0f; F32 iy = 0.0f; F32 iz = 0.0f; F32 iw = 0.0f;
        F32 jx = 0.0f; F32 jy = 1.0f; F32 jz = 0.0f; F32 jw = 0.0f;
        F32 kx = 0.0f; F32 ky = 0.0f; F32 kz = 1.0f; F32 kw = 0.0f;
        F32 tx = 0.0f; F32 ty = 0.0f; F32 tz = 0.0f; F32 tw = 1.0f;

        Mat44() = default;
        Mat44(F32 _ix, F32 _iy, F32 _iz, F32 _iw,
              F32 _jx, F32 _jy, F32 _jz, F32 _jw,
              F32 _kx, F32 _ky, F32 _kz, F32 _kw,
              F32 _tx, F32 _ty, F32 _tz, F32 _tw);
        Mat44(const F32* data);
        Mat44(const Vec3& i, const Vec3& j, const Vec3& k, const Vec3& t);
        Mat44(const Vec4& i, const Vec4& j, const Vec4& k, const Vec4& t);

        F32* operator[](U32 row);
        const F32* operator[](U32 row) const;
        Mat44 operator*(F32 s) const;
        Mat44& operator*=(F32 s);
        Mat44 operator*(const Mat44& other) const;
        Mat44& operator*=(const Mat44& other);

        Vec3 GetIBasis() const;
        Vec3 GetJBasis() const;
        Vec3 GetKBasis() const;
        void SetIBasis(F32 _ix, F32 _iy, F32 _iz);
        void SetJBasis(F32 _jx, F32 _jy, F32 _jz);
        void SetKBasis(F32 _kx, F32 _ky, F32 _kz);
        void SetIBasis(const Vec3& i);
        void SetJBasis(const Vec3& j);
        void SetKBasis(const Vec3& k);

        void Transpose();
        Mat44 GetTransposed() const;

        F32 Determinant() const;

        void Inverse();
        Mat44 GetInversed() const;

        void FastOrthoInverse();
        Mat44 GetFastOrthoInversed() const;

        void Scale(F32 uniformScale);
        void Scale(F32 i, F32 j, F32 k);
        void Scale(const Vec3& ijk);
        void SetScale(F32 uniformScale);
        void SetScale(F32 i, F32 j, F32 k);
        void SetScale(const Vec3& ijk);
        Mat44 GetScaled(F32 uniformScale) const;
        Mat44 GetScaled(F32 i, F32 j, F32 k) const;
        Mat44 GetScaled(const Vec3& ijk) const;

        void Translate(F32 _tx, F32 _ty, F32 _tz);
        void Translate(const Vec3& t);
        void SetTranslation(F32 _tx, F32 _ty, F32 _tz);
        void SetTranslation(const Vec3& t);
        Vec3 GetTranslation() const;
        Mat44 GetTranslated(F32 _tx, F32 _ty, F32 _tz) const;
        Mat44 GetTranslated(const Vec3& t) const;

        void RotateXRads(F32 xRads);
        void RotateYRads(F32 yRads);
        void RotateZRads(F32 zRads);
        void RotateXDegs(F32 xDegs);
        void RotateYDegs(F32 yDegs);
        void RotateZDegs(F32 zDegs);
        void RotateAroundAxisRads(const Vec3& axis, F32 rads);
        void RotateAroundAxisDegs(const Vec3& axis, F32 degs);

        void SetRotationXRads(F32 xRads);
        void SetRotationYRads(F32 yRads);
        void SetRotationZRads(F32 zRads);
        void SetRotationXDegs(F32 xDegs);
        void SetRotationYDegs(F32 yDegs);
        void SetRotationZDegs(F32 zDegs);
        void SetRotationAroundAxisRads(const Vec3& axis, F32 rads);
        void SetRotationAroundAxisDegs(const Vec3& axis, F32 degs);

        Mat44 GetRotatedXRads(F32 xRads) const;
        Mat44 GetRotatedYRads(F32 yRads) const;
        Mat44 GetRotatedZRads(F32 zRads) const;
        Mat44 GetRotatedXDegs(F32 xDegs) const;
        Mat44 GetRotatedYDegs(F32 yDegs) const;
        Mat44 GetRotatedZDegs(F32 zDegs) const;
        Mat44 GetRotatedAroundAxisRads(const Vec3& axis, F32 rads) const;
        Mat44 GetRotatedAroundAxisDegs(const Vec3& axis, F32 degs) const;

        Mat33 GetRotationMat33() const;
        void SetRotationMat33(const Mat33& rotation);

        Mat44 GetRotation() const;
        void SetRotation(const Mat44& rotation);

        static Mat44 CreateScale(F32 uniformScale);
        static Mat44 CreateScale(F32 i, F32 j, F32 k);
        static Mat44 CreateScale(const Vec3& ijk);
        static Mat44 CreateTranslation(F32 _tx, F32 _ty, F32 _tz);
        static Mat44 CreateTranslation(const Vec3& t);
        static Mat44 CreateRotationXRads(F32 xRads);
        static Mat44 CreateRotationYRads(F32 yRads);
        static Mat44 CreateRotationZRads(F32 zRads);
        static Mat44 CreateRotationXDegs(F32 xDegs);
        static Mat44 CreateRotationYDegs(F32 yDegs);
        static Mat44 CreateRotationZDegs(F32 zDegs);
        static Mat44 CreateRotationAroundAxisRads(const Vec3& axis, F32 rads);
        static Mat44 CreateRotationAroundAxisDegs(const Vec3& axis, F32 degs);
    };

    //-------------------------------------------------------------------------
    // General
    //-------------------------------------------------------------------------

    constexpr F32 kPi = 3.1415926535897932384626433832795f;
    constexpr F32 k2Pi = 6.283185307179586476925286766559f;

    bool IsCloseEnough(F32 a, F32 b, F32 acceptableError = FLT_EPSILON);

    inline bool IsCloseEnoughToZero(F32 value, F32 acceptableError = FLT_EPSILON)
    {
        return fabs(value) <= acceptableError;
    };

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

    //-------------------------------------------------------------------------
    // Vec2
    //-------------------------------------------------------------------------
    inline Vec2::Vec2(F32 _x, F32 _y)
        :x(_x)
        ,y(_y)
    {
    }

    inline Vec2 Vec2::operator*(F32 s) const
    {
        return Vec2(x * s, y * s );
    }
    
    inline Vec2 Vec2::operator/(F32 s) const
    {
        return Vec2( x / s, y / s );
    }
    
    inline Vec2& Vec2::operator*=(F32 s)
    {
    	x *= s;
    	y *= s;
    	return *this;
    }
    
    inline Vec2& Vec2::operator/=(F32 s)
    {
    	x /= s;
    	y /= s;
    	return *this;
    }
    
    inline Vec2 Vec2::operator+(const Vec2& other) const
    {
    	return Vec2(x + other.x, y + other.y);
    }
    
    inline Vec2 Vec2::operator-(const Vec2& other) const
    {
    	return Vec2(x - other.x, y - other.y);
    }
    
    inline Vec2& Vec2::operator+=(const Vec2& other)
    {
    	x += other.x;
    	y += other.y;
    	return *this;
    }
    
    inline Vec2& Vec2::operator-=(const Vec2& other)
    {
    	x -= other.x;
    	y -= other.y;
    	return *this;
    }
    
    inline Vec2 Vec2::operator-() const
    {
    	return Vec2(-x, -y);
    }
    
    inline bool Vec2::operator==(const Vec2& other) const
    {
    	return (x == other.x) && (y == other.y);
    }

    inline F32 Vec2::CalcLength() const
    {
        return ::sqrtf(CalcLengthSq());
    }

    inline F32 Vec2::CalcLengthSq() const
    {
    	return (x * x) + (y * y);
    }

    inline void Vec2::SetLength(F32 length)
    {
        Normalize();
        *this *= length;
    }

    inline void Vec2::Normalize()
    {
        // prevent division by zero by checking for zero length and asserting
        F32 lenSq = CalcLengthSq();
        SM_ASSERT(!IsCloseEnoughToZero(lenSq));

        // do normalization now that we know length is not zero
        F32 len = ::sqrtf(lenSq);
        F32 invLen = 1.0f / len;

        *this *= invLen;
    }

    inline Vec2 Vec2::GetNormalized() const
    {
    	Vec2 copy = *this;
        copy.Normalize();
    	return copy;
    }

    inline Vec2 operator*(F32 s, const Vec2& v)
    {
        return v * s;
    }

    inline F32 Dot(const Vec2& a, const Vec2& b)
    {
    	return (a.x * b.x) + (a.y * b.y);
    }

    inline Vec2 PolarToCartesianRads(F32 radians, F32 radius = 1.0f)
    {
        return radius * Vec2(cosf(radians), sinf(radians));
    }

    inline Vec2 PolarToCartesianDegs(F32 deg, F32 radius = 1.0f)
    {
        return radius * Vec2(CosDeg(deg), SinDeg(deg));
    }

    //-------------------------------------------------------------------------
    // Vec3
    //-------------------------------------------------------------------------
    inline Vec3::Vec3(F32 _x, F32 _y, F32 _z)
        :x(_x)
        ,y(_y)
        ,z(_z)
    {
    }

    inline Vec3::Vec3(const Vec2& _xy, F32 _z)
        :x(_xy.x)
        ,y(_xy.y)
        ,z(_z)
    {
    }

    inline Vec3 Vec3::operator*(F32 s) const
    {
        return Vec3(x * s, y * s, z * s);
    }

    inline Vec3 Vec3::operator/(F32 s) const
    {
        F32 invS = 1.0f / s;
        return Vec3(x * invS, y * invS, z * invS);
    }

    inline Vec3& Vec3::operator*=(F32 s)
    {
        x *= s;
        y *= s;
        z *= s;
        return *this;
    }

    inline Vec3& Vec3::operator/=(F32 s)
    {
        F32 inv_s = 1.0f / s;
        x *= inv_s;
        y *= inv_s;
        z *= inv_s;
        return *this;
    }

    inline Vec3 Vec3::operator+(const Vec3& other) const
    {
        return Vec3(x + other.x, y + other.y, z + other.z);
    }

    inline Vec3& Vec3::operator+=(const Vec3& other)
    {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    inline Vec3 Vec3::operator-(const Vec3& other) const
    {
        return Vec3(x - other.x, y - other.y, z - other.z);
    }

    inline Vec3& Vec3::operator-=(const Vec3& other)
    {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }

    inline Vec3 Vec3::operator-() const
    {
        return Vec3(-x, -y, -z);
    }

    inline bool Vec3::operator==(const Vec3& other)
    {
        return (x == other.x) && (y == other.y) && (z == other.z);
    }

    inline F32 Vec3::CalcLength() const
    {
        return ::sqrtf(CalcLengthSq());
    }

    inline F32 Vec3::CalcLengthSq() const
    {
    	return (x * x) + (y * y) + (z * z);
    }

    inline void Vec3::SetLength(F32 length)
    {
        Normalize();
        *this *= length;
    }

    inline void Vec3::Normalize()
    {
        // prevent division by zero by checking for zero length and asserting
        F32 lenSq = CalcLengthSq();
        SM_ASSERT(!IsCloseEnoughToZero(lenSq));

        // do normalization now that we know length is not zero
        F32 len = ::sqrtf(lenSq);
        F32 invLen = 1.0f / len;

        *this *= invLen;
    }

    inline Vec3 Vec3::GetNormalized() const
    {
    	Vec3 copy = *this;
        copy.Normalize();
    	return copy;
    }

    inline Vec3 operator*(F32 s, const Vec3& v)
    {
        return v * s;
    }

    inline F32 Dot(const Vec3& a, const Vec3& b)
    {
        return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
    }

    inline Vec3 Cross(const Vec3& a, const Vec3& b)
    {
        Vec3 cross;
        cross.x = a.y * b.z - a.z * b.y;
        cross.y = a.z * b.x - a.x * b.z;
        cross.z = a.x * b.y - a.y * b.x;
        return cross;
    }

    //-------------------------------------------------------------------------
    // Vec4
    //-------------------------------------------------------------------------
    inline Vec4::Vec4(F32 _x, F32 _y, F32 _z, F32 _w)
        :x(_x)
        ,y(_y)
        ,z(_z)
        ,w(_w)
    {
    }

    inline Vec4::Vec4(const Vec2& _xy, F32 _z, F32 _w)
        :x(_xy.x)
        ,y(_xy.y)
        ,z(_z)
        ,w(_w)
    {
    }

    inline Vec4::Vec4(const Vec3& _xyz, F32 _w)
        :x(_xyz.x)
        ,y(_xyz.y)
        ,z(_xyz.z)
        ,w(_w)
    {
    }

    inline Vec4 Vec4::operator*(F32 s) const
    {
        return Vec4(x * s, y * s, z * s, w * s);
    }

    inline Vec4 Vec4::operator/(F32 s) const
    {
        F32 invS = 1.0f / s;
        return Vec4(x * invS, y * invS, z * invS, w * invS);
    }

    inline Vec4& Vec4::operator*=(F32 s)
    {
        x *= s;
        y *= s;
        z *= s;
        w *= s;
        return *this;
    }

    inline Vec4& Vec4::operator/=(F32 s)
    {
        F32 invS = 1.0f / s;
        x *= invS;
        y *= invS;
        z *= invS;
        w *= invS;
        return *this;
    }

    inline Vec4 Vec4::operator+(const Vec4& other) const
    {
        return Vec4(x + other.x, y + other.y, z + other.z, w + other.w);
    }

    inline Vec4 Vec4::operator-(const Vec4& other) const
    {
        return Vec4(x - other.x, y - other.y, z - other.z, w - other.w);
    }

    inline Vec4& Vec4::operator+=(const Vec4& other)
    {
        x += other.x;
        y += other.y;
        z += other.z;
        w += other.w;
        return *this;
    }

    inline Vec4& Vec4::operator-=(const Vec4& other)
    {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        w -= other.w;
        return *this;
    }

    inline Vec4 Vec4::operator-() const
    {
        return Vec4(-x, -y, -z, -w);
    }

    inline bool Vec4::operator==(const Vec4& other) const
    {
        return (x == other.x) && (y == other.y) && (z == other.z) && (w == other.w);
    }

    inline F32 Vec4::CalcLengthSq() const
    {
        return (x * x) + (y * y) + (z * z) + (w * w);
    }

    inline F32 Vec4::CalcLength()
    {
        return ::sqrtf(CalcLengthSq());
    }

    inline void Vec4::SetLength(F32 length)
    {
        Normalize();
        *this *= length;
    }

    inline void Vec4::Normalize()
    {
        // prevent division by zero by checking for zero length and asserting
        F32 lenSq = CalcLengthSq();
        SM_ASSERT(!IsCloseEnoughToZero(lenSq));

        // do normalization now that we know length is not zero
        F32 length = ::sqrtf(lenSq);
        F32 invLength = 1.0f / length;

        *this *= invLength;
    }

    inline Vec4 Vec4::GetNormalized() const
    {
        Vec4 copy = *this;
        copy.Normalize();
        return copy;
    }

    inline Vec3 Vec4::ToVec3() const
    {
        return Vec3(x, y, z);
    }

    inline Vec4 operator*(F32 s, const Vec4& v)
    {
        return v * s;
    }

    inline F32 Dot(const Vec4& a, const Vec4& b)
    {
        return (a.x * b.x) + (a.y * b.y) + (a.z * b.z) + (a.w * b.w);
    }

    //-------------------------------------------------------------------------
    // IVec2
    //-------------------------------------------------------------------------
    inline IVec2::IVec2(I32 _x, I32 _y)
        :x(_x)
        ,y(_y)
    {
    }

    inline IVec2 IVec2::operator*(I32 s) const
    {
        return IVec2(x * s, y * s );
    }
    
    inline IVec2 IVec2::operator/(I32 s) const
    {
        return IVec2( x / s, y / s );
    }
    
    inline IVec2& IVec2::operator*=(I32 s)
    {
    	x *= s;
    	y *= s;
    	return *this;
    }
    
    inline IVec2& IVec2::operator/=(I32 s)
    {
    	x /= s;
    	y /= s;
    	return *this;
    }
    
    inline IVec2 IVec2::operator+(const IVec2& other) const
    {
    	return IVec2(x + other.x, y + other.y);
    }
    
    inline IVec2 IVec2::operator-(const IVec2& other) const
    {
    	return IVec2(x - other.x, y - other.y);
    }
    
    inline IVec2& IVec2::operator+=(const IVec2& other)
    {
    	x += other.x;
    	y += other.y;
    	return *this;
    }
    
    inline IVec2& IVec2::operator-=(const IVec2& other)
    {
    	x -= other.x;
    	y -= other.y;
    	return *this;
    }
    
    inline IVec2 IVec2::operator-() const
    {
    	return IVec2(-x, -y);
    }
    
    inline bool IVec2::operator==(const IVec2& other) const
    {
    	return (x == other.x) && (y == other.y);
    }

    inline F32 IVec2::CalcLength() const
    {
        return ::sqrtf((F32)CalcLengthSq());
    }

    inline I32 IVec2::CalcLengthSq() const
    {
    	return (x * x) + (y * y);
    }

    inline IVec2 operator*(I32 s, const IVec2& v)
    {
        return v * s;
    }

    inline I32 Dot(const IVec2& a, const IVec2& b)
    {
    	return (a.x * b.x) + (a.y * b.y);
    }

    //-------------------------------------------------------------------------
    // IVec3
    //-------------------------------------------------------------------------
    inline IVec3::IVec3(I32 _x, I32 _y, I32 _z)
        :x(_x)
        ,y(_y)
        ,z(_z)
    {
    }

    inline IVec3::IVec3(const IVec2& _xy, I32 _z)
        :x(_xy.x)
        ,y(_xy.y)
        ,z(_z)
    {
    }

    inline IVec3 IVec3::operator*(I32 s) const
    {
        return IVec3(x * s, y * s, z * s);
    }

    inline IVec3 IVec3::operator/(I32 s) const
    {
        I32 invS = 1.0f / s;
        return IVec3(x * invS, y * invS, z * invS);
    }

    inline IVec3& IVec3::operator*=(I32 s)
    {
        x *= s;
        y *= s;
        z *= s;
        return *this;
    }

    inline IVec3& IVec3::operator/=(I32 s)
    {
        I32 inv_s = 1.0f / s;
        x *= inv_s;
        y *= inv_s;
        z *= inv_s;
        return *this;
    }

    inline IVec3 IVec3::operator+(const IVec3& other) const
    {
        return IVec3(x + other.x, y + other.y, z + other.z);
    }

    inline IVec3& IVec3::operator+=(const IVec3& other)
    {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    inline IVec3 IVec3::operator-(const IVec3& other) const
    {
        return IVec3(x - other.x, y - other.y, z - other.z);
    }

    inline IVec3& IVec3::operator-=(const IVec3& other)
    {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }

    inline IVec3 IVec3::operator-() const
    {
        return IVec3(-x, -y, -z);
    }

    inline bool IVec3::operator==(const IVec3& other)
    {
        return (x == other.x) && (y == other.y) && (z == other.z);
    }

    inline F32 IVec3::CalcLength() const
    {
        return ::sqrtf((F32)CalcLengthSq());
    }

    inline I32 IVec3::CalcLengthSq() const
    {
    	return (x * x) + (y * y) + (z * z);
    }

    inline IVec3 operator*(I32 s, const IVec3& v)
    {
        return v * s;
    }

    inline I32 Dot(const IVec3& a, const IVec3& b)
    {
        return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
    }

    //-------------------------------------------------------------------------
    // Mat33
    //-------------------------------------------------------------------------
    inline Mat33::Mat33(F32 _ix, F32 _iy, F32 _iz,
                        F32 _jx, F32 _jy, F32 _jz,
                        F32 _kx, F32 _ky, F32 _kz)
        :ix(_ix), iy(_iy), iz(_iz)
        ,jx(_jx), jy(_jy), jz(_jz)
        ,kx(_kx), ky(_ky), kz(_kz)
    {
    }

    inline Mat33::Mat33(const Vec3& i, const Vec3& j, const Vec3& k)
        :ix(i.x), iy(i.y), iz(i.z)
        ,jx(j.x), jy(j.y), jz(j.z)
        ,kx(k.x), ky(k.y), kz(k.z)
    {
    }

    inline Mat33::Mat33(F32* data)
    {
        ::memcpy(&ix, data, sizeof(F32) * 9);
    }

    inline F32* Mat33::operator[](U32 row)
    {
        SM_ASSERT(row >= 0 && row < 3);
        F32* data = &ix;
        return &data[row * 3];
    }

    inline const F32* Mat33::operator[](U32 row) const
    {
        SM_ASSERT(row >= 0 && row < 3);
        const F32* data = &ix;
        return &data[row * 3];
    }

    inline Mat33 Mat33::operator*(F32 s) const
    {
        return Mat33(ix * s, iy * s, iz * s, 
                     jx * s, jy * s, jz * s, 
                     kx * s, ky * s, kz * s);
    }

    inline Mat33& Mat33::operator*=(F32 s)
    {
        F32* data = &ix;
        for(int i = 0; i < 9; i++)
        {
            data[i] *= s;
        }
        return *this;
    }

    inline Mat33 Mat33::operator*(const Mat33& other) const
    {
    	Mat33 copy = *this;
    	copy *= other;
    	return copy;
    }

    inline Mat33& Mat33::operator*=(const Mat33& other)
    {
        Mat33 result;

        result.ix = (ix * other.ix) + (iy * other.jx) + (iz * other.kx);
        result.iy = (ix * other.iy) + (iy * other.jy) + (iz * other.ky);
        result.iz = (ix * other.iz) + (iy * other.jz) + (iz * other.kz);

        result.jx = (jx * other.ix) + (jy * other.jx) + (jz * other.kx);
        result.jy = (jx * other.iy) + (jy * other.jy) + (jz * other.ky);
        result.jz = (jx * other.iz) + (jy * other.jz) + (jz * other.kz);

        result.kx = (kx * other.ix) + (ky * other.jx) + (kz * other.kx);
        result.ky = (kx * other.iy) + (ky * other.jy) + (kz * other.ky);
        result.kz = (kx * other.iz) + (ky * other.jz) + (kz * other.kz);

        *this = result;
        return *this;
    }

    inline Vec3 Mat33::GetIBasis() const 
    { 
        return Vec3(ix, iy, iz); 
    }

    inline Vec3 Mat33::GetJBasis() const 
    { 
        return Vec3(jx, jy, jz); 
    }

    inline Vec3 Mat33::GetKBasis() const 
    { 
        return Vec3(kx, ky, kz); 
    }

    inline void Mat33::SetIBasis(F32 _ix, F32 _iy, F32 _iz)
    {
        ix = _ix;
        iy = _iy;
        iz = _iz;
    }

    inline void Mat33::SetJBasis(F32 _jx, F32 _jy, F32 _jz)
    {
        jx = _jx;
        jy = _jy;
        jz = _jz;
    }

    inline void Mat33::SetKBasis(F32 _kx, F32 _ky, F32 _kz)
    {
        kx = _kx;
        ky = _ky;
        kz = _kz;
    }

    inline void Mat33::SetIBasis(const Vec3& i)
    {
        SetIBasis(i.x, i.y, i.z);
    }

    inline void Mat33::SetJBasis(const Vec3& j)
    {
        SetJBasis(j.x, j.y, j.z);
    }

    inline void Mat33::SetKBasis(const Vec3& k)
    {
        SetKBasis(k.x, k.y, k.z);
    }

    inline void Mat33::Transpose()
    {
        F32* data = &ix;
        Swap(data[1], data[3]);
        Swap(data[2], data[6]);
        Swap(data[5], data[7]);
    }

    inline Mat33 Mat33::GetTransposed() const
    {
        Mat33 copy = *this;
        copy.Transpose();
        return copy;
    }

    inline F32 Mat33::Determinant() const
    {
        return ix * (jy * kz - jz * ky) -
               iy * (jx * kz - jz * kx) +
               iz * (jx * ky - jy * kx);
    }

    //-------------------------------------------------------------------------
    // Mat44
    //-------------------------------------------------------------------------
    inline Mat44::Mat44(F32 _ix, F32 _iy, F32 _iz, F32 _iw,
                        F32 _jx, F32 _jy, F32 _jz, F32 _jw,
                        F32 _kx, F32 _ky, F32 _kz, F32 _kw,
                        F32 _tx, F32 _ty, F32 _tz, F32 _tw)
        :ix(_ix), iy(_iy), iz(_iz), iw(_iw)
        ,jx(_jx), jy(_jy), jz(_jz), jw(_jw)
        ,kx(_kx), ky(_ky), kz(_kz), kw(_kw)
        ,tx(_tx), ty(_ty), tz(_tz), tw(_tw)
    {
    }

    inline Mat44::Mat44(const F32* data)
    {
        SM_ASSERT(data);
        memcpy(&ix, data, sizeof(F32) * 16);
    }

    inline Mat44::Mat44(const Vec3& i, const Vec3& j, const Vec3& k, const Vec3& t)
        :Mat44(i.x, i.y, i.z, 0.0f, 
               j.x, j.y, j.z, 0.0f, 
               k.x, k.y, k.z, 0.0f, 
               t.x, t.y, t.z, 1.0f)
    {
    }

    inline Mat44::Mat44(const Vec4& i, const Vec4& j, const Vec4& k, const Vec4& t)
        :Mat44(i.x, i.y, i.z, i.w, 
               j.x, j.y, j.z, j.w, 
               k.x, k.y, k.z, k.w, 
               t.x, t.y, t.z, t.w)
    {
    }

    inline F32* Mat44::operator[](U32 row)
    {
        SM_ASSERT(row >= 0 && row < 4);
        F32* data = &ix;
        return &data[row * 4];
    }

    inline const F32* Mat44::operator[](U32 row) const
    {
        SM_ASSERT(row >= 0 && row < 4);
        const F32* data = &ix;
        return &data[row * 4];
    }

    inline Mat44 Mat44::operator*(F32 s) const
    {
        return Mat44(ix * s, iy * s, iz * s, iw * s, 
                     jx * s, jy * s, jz * s, jw * s, 
                     kx * s, ky * s, kz * s, kw * s, 
                     tx * s, ty * s, tz * s, tw * s);
    }

    inline Mat44& Mat44::operator*=(F32 s)
    {
        F32* data = &ix;
       	for (int idx = 0; idx < 16; idx++)
       	{
       		data[idx] *= s;
       	}
       
       	return *this;
    }

    inline Mat44 Mat44::operator*(const Mat44& other) const
    {
    	Mat44 copy = *this;
    	copy *= other;
    	return copy;
    }

    inline Mat44& Mat44::operator*=(const Mat44& other)
    {
    	Mat44 result;
    
    	result.ix = (ix * other.ix) + (iy * other.jx) + (iz * other.kx) + (iw * other.tx);
    	result.iy = (ix * other.iy) + (iy * other.jy) + (iz * other.ky) + (iw * other.ty);
    	result.iz = (ix * other.iz) + (iy * other.jz) + (iz * other.kz) + (iw * other.tz);
    	result.iw = (ix * other.iw) + (iy * other.jw) + (iz * other.kw) + (iw * other.tw);
    
    	result.jx = (jx * other.ix) + (jy * other.jx) + (jz * other.kx) + (jw * other.tx);
    	result.jy = (jx * other.iy) + (jy * other.jy) + (jz * other.ky) + (jw * other.ty);
    	result.jz = (jx * other.iz) + (jy * other.jz) + (jz * other.kz) + (jw * other.tz);
    	result.jw = (jx * other.iw) + (jy * other.jw) + (jz * other.kw) + (jw * other.tw);
    
    	result.kx = (kx * other.ix) + (ky * other.jx) + (kz * other.kx) + (kw * other.tx);
    	result.ky = (kx * other.iy) + (ky * other.jy) + (kz * other.ky) + (kw * other.ty);
    	result.kz = (kx * other.iz) + (ky * other.jz) + (kz * other.kz) + (kw * other.tz);
    	result.kw = (kx * other.iw) + (ky * other.jw) + (kz * other.kw) + (kw * other.tw);
    
    	result.tx = (tx * other.ix) + (ty * other.jx) + (tz * other.kx) + (tw * other.tx);
    	result.ty = (tx * other.iy) + (ty * other.jy) + (tz * other.ky) + (tw * other.ty);
    	result.tz = (tx * other.iz) + (ty * other.jz) + (tz * other.kz) + (tw * other.tz);
    	result.tw = (tx * other.iw) + (ty * other.jw) + (tz * other.kw) + (tw * other.tw);
    
    	*this = result;
    	return *this;
    }

    Vec3 Mat44::GetIBasis() const
    {
        return Vec3(ix, iy, iz);
    }

    Vec3 Mat44::GetJBasis() const
    {
        return Vec3(jx, jy, jz);
    }

    Vec3 Mat44::GetKBasis() const
    {
        return Vec3(kx, ky, kz);
    }


    inline void Mat44::SetIBasis(F32 _ix, F32 _iy, F32 _iz)
    {
        ix = _ix;
        iy = _iy;
        iz = _iz;
        iw = 0.0f;
    }

    inline void Mat44::SetJBasis(F32 _jx, F32 _jy, F32 _jz)
    {
        jx = _jx;
        jy = _jy;
        jz = _jz;
        jw = 0.0f;
    }

    inline void Mat44::SetKBasis(F32 _kx, F32 _ky, F32 _kz)
    {
        kx = _kx;
        ky = _ky;
        kz = _kz;
        kw = 0.0f;
    }

    inline void Mat44::SetIBasis(const Vec3& i)
    {
        SetIBasis(i.x, i.y, i.z);
    }

    inline void Mat44::SetJBasis(const Vec3& j)
    {
        SetJBasis(j.x, j.y, j.z);
    }

    inline void Mat44::SetKBasis(const Vec3& k)
    {
        SetKBasis(k.x, k.y, k.z);
    }

    inline void Mat44::Transpose()
    {
        F32* data = &ix;
        Swap(data[1], data[4]);
        Swap(data[2], data[8]);
        Swap(data[3], data[12]);
        Swap(data[6], data[9]);
        Swap(data[7], data[13]);
        Swap(data[11], data[14]);
    }

    inline Mat44 Mat44::GetTransposed() const
    {
        Mat44 copy = *this;
        copy.Transpose();
        return copy;
    }

    inline Mat44 Mat44::GetInversed() const
    {
        Mat44 copy = *this;
        copy.Inverse();
        return copy;
    }

    inline void Mat44::FastOrthoInverse()
    {
        // Transpose upper 3x3 matrix
        F32* data = &ix;
        Swap(data[1], data[4]);
        Swap(data[2], data[8]);
        Swap(data[6], data[9]);

        // Negate translation
        tx *= -1.0f;
        ty *= -1.0f;
        tz *= -1.0f;
    }

    inline Mat44 Mat44::GetFastOrthoInversed() const
    {
        Mat44 copy = *this;
        copy.FastOrthoInverse();
        return copy;
    }

    inline void Mat44::Scale(F32 uniformScale)
    {
        Scale(uniformScale, uniformScale, uniformScale);
    }

    inline void Mat44::Scale(F32 i, F32 j, F32 k)
    {
        ix *= i;
        iy *= i;
        iz *= i;
        jx *= j;
        jy *= j;
        jz *= j;
        kx *= k;
        ky *= k;
        kz *= k;
    }

    inline void Mat44::Scale(const Vec3& ijk)
    {
        ix *= ijk.x;
        iy *= ijk.x;
        iz *= ijk.x;
        jx *= ijk.y;
        jy *= ijk.y;
        jz *= ijk.y;
        kx *= ijk.z;
        ky *= ijk.z;
        kz *= ijk.z;
    }

    inline void Mat44::SetScale(F32 uniformScale)
    {
        Vec3 iBasis = *((Vec3*)(&ix));
        Vec3 jBasis = *((Vec3*)(&jx));
        Vec3 kBasis = *((Vec3*)(&kx));

        iBasis.SetLength(uniformScale);
        jBasis.SetLength(uniformScale);
        kBasis.SetLength(uniformScale);
    }

    inline void Mat44::SetScale(F32 i, F32 j, F32 k)
    {
        Vec3 iBasis = *((Vec3*)(&ix));
        Vec3 jBasis = *((Vec3*)(&jx));
        Vec3 kBasis = *((Vec3*)(&kx));

        iBasis.SetLength(i);
        jBasis.SetLength(j);
        kBasis.SetLength(k);
    }

    inline void Mat44::SetScale(const Vec3& ijk)
    {
        Vec3 iBasis = *((Vec3*)(&ix));
        Vec3 jBasis = *((Vec3*)(&jx));
        Vec3 kBasis = *((Vec3*)(&kx));

        iBasis.SetLength(ijk.x);
        jBasis.SetLength(ijk.y);
        kBasis.SetLength(ijk.z);
    }

    inline Mat44 Mat44::GetScaled(F32 uniformScale) const
    {
        Mat44 copy = *this;
        copy.Scale(uniformScale);
        return copy;
    }

    inline Mat44 Mat44::GetScaled(F32 i, F32 j, F32 k) const
    {
        Mat44 copy = *this;
        copy.Scale(i, j, k);
        return copy;
    }

    inline Mat44 Mat44::GetScaled(const Vec3& ijk) const
    {
        Mat44 copy = *this;
        copy.Scale(ijk);
        return copy;
    }

    inline void Mat44::Translate(F32 _tx, F32 _ty, F32 _tz)
    {
        tx += _tx;
        ty += _ty;
        tz += _tz;
    }

    inline void Mat44::Translate(const Vec3& t)
    {
        Translate(t.x, t.y, t.z);
    }
    
    inline void Mat44::SetTranslation(F32 _tx, F32 _ty, F32 _tz)
    {
        tx = _tx;
        ty = _ty;
        tz = _tz;
        tw = 1.0f;
    }

    inline void Mat44::SetTranslation(const Vec3& t)
    {
        SetTranslation(t.x, t.y, t.z);
    }

    inline Vec3 Mat44::GetTranslation() const
    {
        return Vec3(tx, ty, tz);
    }

    inline Mat44 Mat44::GetTranslated(F32 _tx, F32 _ty, F32 _tz) const
    {
        Mat44 copy = *this;
        copy.Translate(_tx, _ty, _tz);
        return copy;
    }

    inline Mat44 Mat44::GetTranslated(const Vec3& t) const
    {
        Mat44 copy = *this;
        copy.Translate(t);
        return copy;
    }

    inline void Mat44::RotateXRads(F32 xRads)
    {
        *this *= Mat44::CreateRotationYRads(xRads);
    }

    inline void Mat44::RotateYRads(F32 yRads)
    {
        *this *= Mat44::CreateRotationYRads(yRads);
    }

    inline void Mat44::RotateZRads(F32 zRads)
    {
        *this *= Mat44::CreateRotationYRads(zRads);
    }

    inline void Mat44::RotateXDegs(F32 xDegs)
    {
        RotateXRads(DegToRad(xDegs));
    }

    inline void Mat44::RotateYDegs(F32 yDegs)
    {
        RotateYRads(DegToRad(yDegs));
    }

    inline void Mat44::RotateZDegs(F32 zDegs)
    {
        RotateZRads(DegToRad(zDegs));
    }

    inline void Mat44::RotateAroundAxisRads(const Vec3& axis, F32 rads)
    {
        Vec3 i = axis.GetNormalized();
        Vec3 j = Cross(Vec3::WORLD_UP, axis).GetNormalized();
        Vec3 k = Cross(i, j);
        Mat44 rotWorldBasis = Mat44(i, j, k, Vec3::ZERO);
        Mat44 rotWorldLocal = rotWorldBasis.GetTransposed();
        Mat44 localRot = Mat44::CreateRotationXRads(rads);

        *this *= (rotWorldLocal * localRot * rotWorldBasis);
    }

    inline void Mat44::RotateAroundAxisDegs(const Vec3& axis, F32 degs)
    {
        RotateAroundAxisRads(axis, DegToRad(degs));
    }

    inline void Mat44::SetRotationXRads(F32 xRads)
    {
        F32 cosRads = ::cosf(xRads);
        F32 sinRads = ::sinf(xRads);
        SetIBasis(1.0f, 0.0f, 0.0f);
        SetJBasis(0.0f, cosRads, sinRads);
        SetKBasis(0.0f, -sinRads, cosRads);
    }

    inline void Mat44::SetRotationYRads(F32 yRads)
    {
        F32 cosRads = ::cosf(yRads);
        F32 sinRads = ::sinf(yRads);
        SetIBasis(cosRads, 0.0f, -sinRads);
        SetJBasis(0.0f, 1.0f, 0.0f);
        SetKBasis(sinRads, 0.0f, cosRads);
    }

    inline void Mat44::SetRotationZRads(F32 zRads)
    {
        F32 cosRads = ::cosf(zRads);
        F32 sinRads = ::sinf(zRads);
        SetIBasis(cosRads, sinRads, 0.0f);
        SetJBasis(-sinRads, cosRads, 0.0f);
        SetKBasis(0.0f, 0.0f, 1.0f);
    }

    inline void Mat44::SetRotationXDegs(F32 xDegs)
    {
        SetRotationXRads(DegToRad(xDegs));
    }

    inline void Mat44::SetRotationYDegs(F32 yDegs)
    {
        SetRotationYRads(DegToRad(yDegs));
    }

    inline void Mat44::SetRotationZDegs(F32 zDegs)
    {
        SetRotationZRads(DegToRad(zDegs));
    }

    inline void Mat44::SetRotationAroundAxisRads(const Vec3& axis, F32 rads)
    {
        Vec3 i = axis.GetNormalized();
        Vec3 j = Cross(Vec3::WORLD_UP, axis).GetNormalized();
        Vec3 k = Cross(i, j);
        Mat44 rotWorldBasis = Mat44(i, j, k, Vec3::ZERO);
        Mat44 rotWorldLocal = rotWorldBasis.GetTransposed();
        Mat44 localRot = Mat44::CreateRotationXRads(rads);

        SetRotation(rotWorldLocal * localRot * rotWorldBasis);
    }

    inline void Mat44::SetRotationAroundAxisDegs(const Vec3& axis, F32 degs)
    {
        SetRotationAroundAxisRads(axis, DegToRad(degs));
    }

    inline Mat44 Mat44::GetRotatedXRads(F32 xRads) const
    {
        return *this * Mat44::CreateRotationXRads(xRads);    
    }

    inline Mat44 Mat44::GetRotatedYRads(F32 yRads) const
    {
        return *this * Mat44::CreateRotationYRads(yRads);    
    }

    inline Mat44 Mat44::GetRotatedZRads(F32 zRads) const
    {
        return *this * Mat44::CreateRotationZRads(zRads);    
    }

    inline Mat44 Mat44::GetRotatedXDegs(F32 xDegs) const
    {
        return *this * Mat44::CreateRotationXDegs(xDegs);    
    }

    inline Mat44 Mat44::GetRotatedYDegs(F32 yDegs) const
    {
        return *this * Mat44::CreateRotationYDegs(yDegs);    
    }

    inline Mat44 Mat44::GetRotatedZDegs(F32 zDegs) const
    {
        return *this * Mat44::CreateRotationZDegs(zDegs);    
    }

    inline Mat44 Mat44::GetRotatedAroundAxisRads(const Vec3& axis, F32 rads) const
    {
        return *this * Mat44::CreateRotationAroundAxisRads(axis, rads);    
    }

    inline Mat44 Mat44::GetRotatedAroundAxisDegs(const Vec3& axis, F32 degs) const
    {
        return *this * Mat44::CreateRotationAroundAxisDegs(axis, degs);    
    }

    inline Mat33 Mat44::GetRotationMat33() const
    {
        return Mat33(GetIBasis(), GetJBasis(), GetKBasis());
    }

    inline void Mat44::SetRotationMat33(const Mat33& rotation)
    {
        SetIBasis(rotation.GetIBasis());
        SetJBasis(rotation.GetJBasis());
        SetKBasis(rotation.GetKBasis());
    }

    inline Mat44 Mat44::GetRotation() const
    {
        return Mat44(GetIBasis(), GetJBasis(), GetKBasis(), Vec3::ZERO);
    }

    inline void Mat44::SetRotation(const Mat44& rotation)
    {
        SetIBasis(rotation.GetIBasis());
        SetJBasis(rotation.GetJBasis());
        SetKBasis(rotation.GetKBasis());
    }

    inline Mat44 Mat44::CreateScale(F32 uniformScale)
    {
        Mat44 scale;
        scale.SetScale(uniformScale);
        return scale;
    }

    inline Mat44 Mat44::CreateScale(F32 i, F32 j, F32 k)
    {
        Mat44 scale;
        scale.SetScale(i, j , k);
        return scale;
    }

    inline Mat44 Mat44::CreateScale(const Vec3& ijk)
    {
        Mat44 scale;
        scale.SetScale(ijk);
        return scale;
    }

    inline Mat44 Mat44::CreateTranslation(F32 _tx, F32 _ty, F32 _tz)
    {
        Mat44 translation;
        translation.SetScale(_tx, _ty, _tz);
        return translation;
    }

    inline Mat44 Mat44::CreateTranslation(const Vec3& t)
    {
        Mat44 translation;
        translation.SetScale(t);
        return translation;
    }

    inline Mat44 Mat44::CreateRotationXRads(F32 xRads)
    {
        Mat44 rot;
        rot.SetRotationXRads(xRads);
        return rot;
    }

    inline Mat44 Mat44::CreateRotationYRads(F32 yRads)
    {
        Mat44 rot;
        rot.SetRotationYRads(yRads);
        return rot;
    }

    inline Mat44 Mat44::CreateRotationZRads(F32 zRads)
    {
        Mat44 rot;
        rot.SetRotationZRads(zRads);
        return rot;
    }

    inline Mat44 Mat44::CreateRotationXDegs(F32 xDegs)
    {
        Mat44 rot;
        rot.SetRotationXDegs(xDegs);
        return rot;
    }

    inline Mat44 Mat44::CreateRotationYDegs(F32 yDegs)
    {
        Mat44 rot;
        rot.SetRotationYDegs(yDegs);
        return rot;
    }

    inline Mat44 Mat44::CreateRotationZDegs(F32 zDegs)
    {
        Mat44 rot;
        rot.SetRotationZDegs(zDegs);
        return rot;
    }

    inline Mat44 Mat44::CreateRotationAroundAxisRads(const Vec3& axis, F32 rads)
    {
        Mat44 rot;
        rot.RotateAroundAxisRads(axis, rads);
        return rot;
    }

    inline Mat44 Mat44::CreateRotationAroundAxisDegs(const Vec3& axis, F32 degs)
    {
        Mat44 rot;
        rot.RotateAroundAxisDegs(axis, degs);
        return rot;
    }

    inline Vec4 operator*(const Vec4& v, const Mat44& mat)
    {
        Vec4 result;
        result.x = (v.x * mat.ix) + (v.y * mat.jx) + (v.z * mat.kx) + (v.w * mat.tx);
        result.y = (v.x * mat.iy) + (v.y * mat.jy) + (v.z * mat.ky) + (v.w * mat.ty);
        result.z = (v.x * mat.iz) + (v.y * mat.jz) + (v.z * mat.kz) + (v.w * mat.tz);
        result.w = (v.x * mat.iw) + (v.y * mat.jw) + (v.z * mat.kw) + (v.w * mat.tw);
        return result;
    }
}
