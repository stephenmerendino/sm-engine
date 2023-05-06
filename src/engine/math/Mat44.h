#pragma once
#pragma warning(disable:4201)

#include "engine/core/Common.h"
#include "engine/math/Vec4.h"
#include "engine/math/Vec3.h"

#include <memory.h>

// World Coordinate System
//	Right Handed Z-Up 
//		X : Forward
//		Y : Left
//		Z : Up

// Camera Coordinate System
//	Left Handed Y-Up 
//		X : Right 
//		Y : Up 
//		Z : Forward 


class Mat44
{
public:
	enum { NumElements = 16 };

	union
	{
		struct
		{
			Vec4 m_i;
			Vec4 m_j;
			Vec4 m_k;
			Vec4 m_t;
		};

		struct
		{
			F32 m_ix, m_iy, m_iz, m_iw;
			F32 m_jx, m_jy, m_jz, m_jw;
			F32 m_kx, m_ky, m_kz, m_kw;
			F32 m_tx, m_ty, m_tz, m_tw;
		};

		F32 m_data[NumElements];
	};

	inline Mat44(){};
	inline Mat44(const Vec4& i, const Vec4& j, const Vec4& k, const Vec4& t);
	inline Mat44(F32 ix, F32 iy, F32 iz, F32 iw,
				 F32 jx, F32 jy, F32 jz, F32 jw,
				 F32 kx, F32 ky, F32 kz, F32 kw,
				 F32 tx, F32 ty, F32 tz, F32 tw);
	inline Mat44(F32 *data);

	inline Mat44 operator*(F32 s) const;
	inline Mat44& operator*=(F32 s);
	inline Mat44 operator*(const Mat44& other) const;
	Mat44& operator*=(const Mat44& other);

	inline F32 GetElement(U32 row, U32 column) const;
	inline void SetElement(U32 row, U32 column, F32 value);
	inline F32 GetElement(U32 index) const;
	inline void SetElement(U32 index, F32 value);

	void Transpose();
	inline Mat44 GetTransposed() const;

	F32 CalcDeterminant() const;
	F32 CalcCofactor(U32 row, U32 column) const;
	void Inverse();
	inline Mat44 GetInversed() const;
	void FastOrthogonalInverse();
	inline Mat44 GetFastOrthogonalInversed() const;

	void Translate(const Vec3& t);
	void SetTranslation(const Vec3& t);
	Vec3 GetTranslation() const;
	static Mat44 MakeTranslation(const Vec3& t);

	void RotateXDeg(F32 xDegrees);
	void RotateYDeg(F32 yDegrees);
	void RotateZDeg(F32 zDegrees);
	void RotateXRad(F32 xRadians);
	void RotateYRad(F32 yRadians);
	void RotateZRad(F32 zRadians);

	static Mat44 MakeRotationXDeg(F32 xDegrees);
	static Mat44 MakeRotationYDeg(F32 yDegrees);
	static Mat44 MakeRotationZDeg(F32 zDegrees);
	static Mat44 MakeRotationXRad(F32 xRadians);
	static Mat44 MakeRotationYRad(F32 yRadians);
	static Mat44 MakeRotationZRad(F32 zRadians);

	Mat44 GetRotation();
	void SetRotation(const Mat44& rotation);

	void Scale(F32 uniformScale);
	void Scale(F32 iScale,  F32 jScale, F32 kScale);
	void Scale(const Vec3& scaleFactors);

	inline Mat44 GetScaled(F32 uniformScale) const;
	inline Mat44 GetScaled(F32 iScale,  F32 jScale, F32 kScale) const;
	inline Mat44 GetScaled(const Vec3& scaleFactors) const;

	void SetIBasis(const Vec3& iBasis);
	void SetJBasis(const Vec3& jBasis);
	void SetKBasis(const Vec3& kBasis);

	Vec3 GetIBasis() const;
	Vec3 GetJBasis() const;
	Vec3 GetKBasis() const;

	Vec3 TransformVector(const Vec3& v) const;
	Vec3 TransformPoint(const Vec3& p) const;

	static Mat44 IDENTITY;
};

// perspective projection
Mat44 CreatePerspectiveProjection(F32 verticalFoVDegrees, F32 nearPlane, F32 farPlane, F32 aspect);

// orthographic projection

inline 
Mat44::Mat44(const Vec4& i, const Vec4& j, const Vec4& k, const Vec4& t)
	:m_i(i)
	,m_j(j)
	,m_k(k)
	,m_t(t)
{
}

inline 
Mat44::Mat44(F32 ix, F32 iy, F32 iz, F32 iw,
			 F32 jx, F32 jy, F32 jz, F32 jw,
			 F32 kx, F32 ky, F32 kz, F32 kw,
			 F32 tx, F32 ty, F32 tz, F32 tw)
	:m_i(ix, iy, iz, iw)
	,m_j(jx, jy, jz, jw)
	,m_k(kx, ky, kz, kw)
	,m_t(tx, ty, tz, tw)
{
}

inline 
Mat44::Mat44(F32 *data)
{
	ASSERT(nullptr != data);
	memcpy(m_data, data, sizeof(F32) * NumElements);
}

inline 
Mat44 Mat44::operator*(F32 s) const
{
	Mat44 copy = *this;
	copy *= s;
	return copy;
}

inline 
Mat44& Mat44::operator*=(F32 s)
{
	for (int i = 0; i < NumElements; i++)
	{
		m_data[i] *= s;
	}

	return *this;
}

inline 
Mat44 Mat44::operator*(const Mat44& other) const
{
	Mat44 copy = *this;
	copy *= other;
	return copy;
}

inline 
F32 Mat44::GetElement(U32 row, U32 column) const
{
	ASSERT(row >= 0 && row < 4);
	ASSERT(column >= 0 && column < 4);
	return m_data[row * 4 + column];
}

inline 
void Mat44::SetElement(U32 row, U32 column, F32 value)
{
	ASSERT(row >= 0 && row < 4);
	ASSERT(column >= 0 && column < 4);
	m_data[row * 4 + column] = value;
}

inline 
F32 Mat44::GetElement(U32 index) const
{
	ASSERT(index >= 0 && index < NumElements);
	return m_data[index];
}

inline 
void Mat44::SetElement(U32 index, F32 value)
{
	ASSERT(index >= 0 && index < NumElements);
	m_data[index] = value;
}

inline 
Mat44 Mat44::GetTransposed() const
{
	Mat44 copy = *this;
	copy.Transpose();
	return copy;
}

inline 
Mat44 Mat44::GetInversed() const
{
	Mat44 copy = *this;
	copy.Inverse();
	return copy;
}

inline 
Mat44 Mat44::GetFastOrthogonalInversed() const
{
	Mat44 copy = *this;
	copy.FastOrthogonalInverse();
	return copy;
}

inline 
Mat44 Mat44::GetScaled(F32 uniformScale) const
{
	Mat44 scaled = *this;
	scaled.Scale(uniformScale);
	return scaled;
}

inline 
Mat44 Mat44::GetScaled(F32 iScale, F32 jScale, F32 kScale) const
{
	Mat44 scaled = *this;
	scaled.Scale(iScale, jScale, kScale);
	return scaled;
}

inline 
Mat44 Mat44::GetScaled(const Vec3& scaleFactors) const
{
	Mat44 scaled = *this;
	scaled.Scale(scaleFactors);
	return scaled;
}

inline
Mat44 operator*(F32 s, const Mat44& m)
{
	return m * s;
}

Vec4 operator*(const Vec4& v, const Mat44& mat);
