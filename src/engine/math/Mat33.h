#pragma once
#pragma warning(disable:4201)

#include "engine/core/Common.h"
#include "engine/math/Vec4.h"
#include "engine/math/Vec3.h"

#include <memory.h>

class Mat33
{
public:
	enum { NumElements = 9 };

	union
	{
		struct
		{
			Vec3 m_i;
			Vec3 m_j;
			Vec3 m_k;
		};

		struct
		{
			F32 m_ix, m_iy, m_iz;
			F32 m_jx, m_jy, m_jz;
			F32 m_kx, m_ky, m_kz;
		};

		F32 m_data[NumElements];
	};

	inline Mat33(){};
	inline Mat33(const Vec3& i, const Vec3& j, const Vec3& k);
	inline Mat33(F32 ix, F32 iy, F32 iz,
				 F32 jx, F32 jy, F32 jz,
				 F32 kx, F32 ky, F32 kz);
	inline Mat33(F32 *data);

	inline Mat33 operator*(F32 s) const;
	inline Mat33& operator*=(F32 s);
	inline Mat33 operator*(const Mat33& other) const;
	Mat33& operator*=(const Mat33& other);

	inline F32 GetElement(U32 row, U32 column) const;
	inline void SetElement(U32 row, U32 column, F32 value);
	inline F32 GetElement(U32 index) const;
	inline void SetElement(U32 index, F32 value);

	void Transpose();
	inline Mat33 GetTransposed() const;

	F32 CalcDeterminant() const;

	static Mat33 IDENTITY;
};

inline 
Mat33::Mat33(const Vec3& i, const Vec3& j, const Vec3& k)
	:m_i(i)
	,m_j(j)
	,m_k(k)
{
}

inline 
Mat33::Mat33(F32 ix, F32 iy, F32 iz,
			 F32 jx, F32 jy, F32 jz,
			 F32 kx, F32 ky, F32 kz)
	:m_i(ix, iy, iz)
	,m_j(jx, jy, jz)
	,m_k(kx, ky, kz)
{
}

inline 
Mat33::Mat33(F32 *data)
{
	ASSERT(nullptr != data);
	memcpy(m_data, data, sizeof(F32) * NumElements);
}

inline 
Mat33 Mat33::operator*(F32 s) const
{
	Mat33 copy = *this;
	copy *= s;
	return copy;
}

inline 
Mat33& Mat33::operator*=(F32 s)
{
	for (int i = 0; i < NumElements; i++)
	{
		m_data[i] *= s;
	}

	return *this;
}

inline 
Mat33 Mat33::operator*(const Mat33& other) const
{
	Mat33 copy = *this;
	copy *= other;
	return copy;
}

inline 
F32 Mat33::GetElement(U32 row, U32 column) const
{
	ASSERT(row >= 0 && row < 3);
	ASSERT(column >= 0 && column < 3);
	return m_data[row * 4 + column];
}

inline 
void Mat33::SetElement(U32 row, U32 column, F32 value)
{
	ASSERT(row >= 0 && row < 3);
	ASSERT(column >= 0 && column < 3);
	m_data[row * 4 + column] = value;
}

inline 
F32 Mat33::GetElement(U32 index) const
{
	ASSERT(index >= 0 && index < NumElements);
	return m_data[index];
}

inline 
void Mat33::SetElement(U32 index, F32 value)
{
	ASSERT(index >= 0 && index < NumElements);
	m_data[index] = value;
}

inline 
Mat33 Mat33::GetTransposed() const
{
	Mat33 copy = *this;
	copy.Transpose();
	return copy;
}

inline
Mat33 operator*(F32 s, const Mat33& m)
{
	return m * s;
}

Vec3 operator*(const Vec3& v, const Mat33& mat);

