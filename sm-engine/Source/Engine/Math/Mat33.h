#pragma once
#pragma warning(disable:4201)

#include "Engine/Core/Utils.h"
#include "Engine/Math/Vec3_old.h"

#include <memory.h>

class Mat33
{
public:
	inline Mat33();
	inline Mat33(const Vec3& inI, const Vec3& inJ, const Vec3& inK);
	inline Mat33(F32 ix, F32 iy, F32 iz,
				 F32 jx, F32 jy, F32 jz,
				 F32 kx, F32 ky, F32 kz);
	inline Mat33(F32* data);

	F32* operator[](U32 row);
	const F32* operator[](U32 row) const;
	inline Mat33 operator*(F32 s) const;
	inline Mat33& operator*=(F32 s);
	inline Mat33 operator*(const Mat33& b) const;
	inline Mat33& operator*=(const Mat33& other);

	inline void Transpose();
	inline Mat33 Transposed() const;
	inline F32 CalcDeterminant() const;

	union
	{
		struct
		{
			Vec3 i;
			Vec3 j;
			Vec3 k;
		};

		struct
		{
			F32 ix, iy, iz;
			F32 jx, jy, jz;
			F32 kx, ky, kz;
		};

		F32 data[9];
	};

	static const Mat33 IDENTITY;
};

inline Mat33::Mat33()
	:i(Vec3::ZERO)
	,j(Vec3::ZERO)
	,k(Vec3::ZERO)
{
}

inline Mat33::Mat33(const Vec3& inI, const Vec3& inJ, const Vec3& inK)
	:i(inI)
	,j(inJ)
	,k(inK)
{
}

inline Mat33::Mat33(F32 ix, F32 iy, F32 iz,
					F32 jx, F32 jy, F32 jz,
					F32 kx, F32 ky, F32 kz)
	:i(ix, iy, iz)
	,j(jx, jy, jz)
	,k(kx, ky, kz)
{
}

inline Mat33::Mat33(F32* data)
	:i(Vec3::ZERO)
	,j(Vec3::ZERO)
	,k(Vec3::ZERO)
{
	SM_ASSERT(nullptr != data);
	memcpy(data, data, sizeof(Mat33));
}

inline F32* Mat33::operator[](U32 row)
{
	SM_ASSERT(row >= 0 && row <= 2);
	return &data[row * 3];
}

inline const F32* Mat33::operator[](U32 row) const
{
	SM_ASSERT(row >= 0 && row <= 2);
	return &data[row * 3];
}

inline Mat33 Mat33::operator*(F32 s) const
{
	return Mat33(i * s, j * s, k * s);
}

inline Mat33& Mat33::operator*=(F32 s)
{
	for (int idx = 0; idx < 9; idx++)
	{
		data[idx] *= s;
	}

	return *this;
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

inline Mat33 Mat33::operator*(const Mat33& other) const
{
	Mat33 copy = *this;
	copy *= other;
	return copy;
}

inline void Mat33::Transpose()
{
	Swap(data[1], data[3]);
	Swap(data[2], data[6]);
	Swap(data[5], data[7]);
}

inline Mat33 Mat33::Transposed() const
{
	Mat33 copy = *this;
	copy.Transpose();
	return copy;
}

inline F32 Mat33::CalcDeterminant() const
{
	return ix * (jy * kz - jz * ky) -
		   iy * (jx * kz - jz * kx) +
		   iz * (jx * ky - jy * kx);
}

inline Mat33 operator*(F32 s, const Mat33& m)
{
	return m * s;
}

inline Vec3 operator*(const Vec3& v, const Mat33& m)
{
	Vec3 result;
	result.x = (v.x * m.ix) + (v.y * m.jx) + (v.z * m.kx);
	result.y = (v.x * m.iy) + (v.y * m.jy) + (v.z * m.ky);
	result.z = (v.x * m.iz) + (v.y * m.jz) + (v.z * m.kz);
	return result;
}
