#pragma once
#pragma warning(disable:4201)

#include "Engine/Core/Types.h"
#include "Engine/Math/MathUtils.h"

class IVec2
{
public:
	inline IVec2();
	inline IVec2(I32 inX, I32 inY);

	inline IVec2 operator*(I32 scale) const;
	inline IVec2 operator/(I32 scale) const;
	inline IVec2& operator*=(I32 scale);
	inline IVec2& operator/=(I32 scale);

	inline IVec2 operator+(const IVec2& other) const;
	inline IVec2 operator-(const IVec2& other) const;
	inline IVec2& operator+=(const IVec2& other);
	inline IVec2& operator-=(const IVec2& other);

	inline IVec2 operator-() const;
	inline bool operator==(const IVec2& other) const;

	inline I32 CalcLengthSq() const;
	inline F32 CalcLength() const;

	union
	{
		struct
		{
			I32 x;
			I32 y;
		};

		I32 data[2];
	};

	static const IVec2 ZERO;
};

inline IVec2::IVec2()
	:x(0)
	,y(0)
{
}

inline IVec2::IVec2(I32 inX, I32 inY)
	:x(inX)
	,y(inY)
{
}

inline IVec2 IVec2::operator*(I32 scale) const
{
	return IVec2(x * scale, y * scale);
}

inline IVec2 IVec2::operator/(I32 scale) const
{
	return IVec2(x / scale, y / scale);
}

inline IVec2& IVec2::operator*=(I32 scale)
{
	x *= scale;
	y *= scale;
	return *this;
}

inline IVec2& IVec2::operator/=(I32 scale)
{
	x /= scale;
	y /= scale;
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

inline I32 IVec2::CalcLengthSq() const
{
	return (x * x) + (y * y);
}

inline F32 IVec2::CalcLength() const
{
	return sqrtf(CalcLengthSq());
}

inline I32 Dot(const IVec2& a, const IVec2& b)
{
	return (a.x * b.x) + (a.y * b.y);
}

inline F32 Dist(const IVec2& a, const IVec2& b)
{
	return (a - b).CalcLength();
}

inline I32 DistSq(const IVec2& a, const IVec2& b)
{
	return (a - b).CalcLengthSq();
}

inline IVec2 operator*(I32 s, const IVec2& iv)
{
	return IVec2(iv.x * s, iv.y * s);
}
