#pragma once
#include "Engine/Math/MathUtils.h"
#pragma warning(disable:4201)

#include "Engine/Core/Utils.h"
#include "Engine/Math/Mat33.h"
#include "Engine/Math/Vec4.h"
#include "Engine/Math/Vec3.h"

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
	inline Mat44();
	inline Mat44(const Vec4& inI, const Vec4& inJ, const Vec4& inK, const Vec4& inT);
	inline Mat44(F32 ix, F32 iy, F32 iz, F32 iw,
				 F32 jx, F32 jy, F32 jz, F32 jw,
				 F32 kx, F32 ky, F32 kz, F32 kw,
				 F32 tx, F32 ty, F32 tz, F32 tw);
	inline Mat44(F32* data);
	inline Mat44(const Vec3& inI, const Vec3& inJ, const Vec3& inK);
	inline Mat44(const Vec3& inI, const Vec3& inJ, const Vec3& inK, const Vec3& inT);

	inline F32* operator[](U32 row);
	inline const F32* operator[](U32 row) const;

	inline Mat44 operator*(F32 s) const;
	inline Mat44& operator*=(F32 s);
	Mat44 operator*(const Mat44& other) const;
	Mat44& operator*=(const Mat44& other);

	inline void Transpose();
	inline Mat44 Transposed() const;
	inline F32 CalcDeterminant() const;
	inline void Inverse();
	inline Mat44 Inversed() const;
	inline void FastOrthoInverse();
	inline Mat44 FastOrthoInversed() const;

	inline void Scale(F32 uniformScale);
	inline void Scale(F32 iScale, F32 jScale, F32 kScale);
	inline void Scale(const Vec3& ijkScale);
	inline Mat44 Scaled(F32 uniformScale) const;
	inline Mat44 Scaled(F32 iScale, F32 jScale, F32 kScale) const;
	inline Mat44 Scaled(const Vec3& ijkScale) const;

	inline void SetTranslation(const Vec3& inT);
	inline void Translate(const Vec3& inT);
	inline static Mat44 CreateTranslation(const Vec3& t);

	inline void RotateXRads(F32 xRads);
	inline void RotateYRads(F32 yRads);
	inline void RotateZRads(F32 zRads);
	inline void RotateXDegs(F32 xDegs);
	inline void RotateYDegs(F32 yDegs);
	inline void RotateZDegs(F32 zDegs);
	inline static Mat44 CreateRotationXRads(F32 xRads);
	inline static Mat44 CreateRotationYRads(F32 yRads);
	inline static Mat44 CreateRotationZRads(F32 zRads);
	inline static Mat44 CreateRotationXDegs(F32 xDegs);
	inline static Mat44 CreateRotationYDegs(F32 yDegs);
	inline static Mat44 CreateRotationZDegs(F32 zDegs);

	inline void RotateAroundAxisRads(const Vec3& axis, F32 rads);
	inline void RotateAroundAxisDegs(const Vec3& axis, F32 degs);
	inline static Mat44 CreateRotationArounAxisRads(const Vec3& axis, F32 rads);
	inline static Mat44 CreateRotationAroundAxisDegs(const Vec3& axis, F32 degs);

	inline Mat44 GetRotation() const;
	inline void SetRotation(const Mat44& rotation);

	inline Vec3 TransformVec(const Vec3& v) const;
	inline Vec3 TransformPoint(const Vec3& p) const;
	inline void TransformInModelSpace(const Mat44& transform);
	inline Mat44 GetTransformedInModelSpace(const Mat44& transform) const;

	inline static Mat44 CreatePerspectiveProjection(F32 verticalFovDeg, F32 nearPlane, F32 farPlane, F32 aspect);

	inline static Mat44 CreateBasisFromI(const Vec3& inI);
	inline static Mat44 CreateBasisFromJ(const Vec3& inJ);
	inline static Mat44 CreateBasisFromK(const Vec3& inK);


	union
	{
		struct
		{
			Vec4 i;
			Vec4 j;
			Vec4 k;
			Vec4 t;
		};

		struct
		{
			F32 ix, iy, iz, iw;
			F32 jx, jy, jz, jw;
			F32 kx, ky, kz, kw;
			F32 tx, ty, tz, tw;
		};

		F32 data[16];
	};

	static const Mat44 IDENTITY;
};

inline Mat44::Mat44()
	:i(Vec4::ZERO)
	,j(Vec4::ZERO)
	,k(Vec4::ZERO)
	,t(Vec4::ZERO)
{
}

inline Mat44::Mat44(const Vec4& inI, const Vec4& inJ, const Vec4& inK, const Vec4& inT)
	:i(inI)
	,j(inJ)
	,k(inK)
	,t(inT)
{
}

inline Mat44::Mat44(F32 ix, F32 iy, F32 iz, F32 iw,
					F32 jx, F32 jy, F32 jz, F32 jw,
					F32 kx, F32 ky, F32 kz, F32 kw,
					F32 tx, F32 ty, F32 tz, F32 tw)
	:i(ix, iy, iz, iw)
	,j(jx, jy, jz, jw)
	,k(kx, ky, kz, kw)
	,t(tx, ty, tz, tw)
{
}

inline Mat44::Mat44(F32* data)
	:i(Vec4::ZERO)
	,j(Vec4::ZERO)
	,k(Vec4::ZERO)
	,t(Vec4::ZERO)
{
	SM_ASSERT(nullptr != data);
	memcpy(data, data, sizeof(Mat44));
}

inline Mat44::Mat44(const Vec3& inI, const Vec3& inJ, const Vec3& inK)
	:i(inI, 0.0f)
	,j(inJ, 0.0f)
	,k(inK, 0.0f)
	,t(0.0f, 0.0f, 0.0f, 1.0f)
{
}

inline Mat44::Mat44(const Vec3& inI, const Vec3& inJ, const Vec3& inK, const Vec3& inT)
	:i(inI, 0.0f)
	,j(inJ, 0.0f)
	,k(inK, 0.0f)
	,t(inT, 1.0f)
{
}

inline F32* Mat44::operator[](U32 row)
{
	SM_ASSERT(row >= 0 && row <= 3);
	return &data[row * 4];
}

inline const F32* Mat44::operator[](U32 row) const
{
	SM_ASSERT(row >= 0 && row <= 3);
	return &data[row * 4];
}

inline Mat44 Mat44::operator*(F32 s) const
{
	return Mat44(i * s, j * s, k * s, t * s);
}

inline Mat44& Mat44::operator*=(F32 s)
{
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

inline void Mat44::Transpose()
{
	Swap(data[1], data[4]);
	Swap(data[2], data[8]);
	Swap(data[3], data[12]);
	Swap(data[6], data[9]);
	Swap(data[7], data[13]);
	Swap(data[11], data[14]);
}

inline Mat44 Mat44::Transposed() const
{
	Mat44 copy = *this;
	copy.Transpose();
	return copy;
}

inline static F32 CalcCofactor(const Mat44& m, U32 row, U32 column)
{
	Mat33 submatrix;

	// Build Mat33 submatrix by removing the row and column we are getting cofactor for
	U32 curRow = 0;
	for (U32 srcRow = 0; srcRow < 4; ++srcRow)
	{
		if (srcRow == row)
		{
			continue;
		}

		U32 curCol = 0;
		for (U32 srcCol = 0; srcCol < 4; ++srcCol)
		{
			if (srcCol == column)
			{
				continue;
			}

			submatrix[curRow][curCol] = m[srcRow][srcCol];

			curCol++;
		}

		curRow++;
	}

	// Calculate determinant of submatrix
	F32 subDeterminant = submatrix.CalcDeterminant();

	// Use correct sign based on row and column
	if ((row + column) % 2 != 0)
	{
		subDeterminant = -subDeterminant;
	}

	return subDeterminant;
}

inline F32 Mat44::CalcDeterminant() const
{
	return ix * CalcCofactor(*this, 0, 0) +
		   iy * CalcCofactor(*this, 0, 1) +
		   iz * CalcCofactor(*this, 0, 2) +
		   iw * CalcCofactor(*this, 0, 3);
}

inline void Mat44::Inverse()
{
	// Cramer's method of inverse determinant multiplied by adjoint matrix
	F32 invDeterminant = 1.0f / CalcDeterminant();

	// Build matrix of cofactors
	Mat44 cofactorMat;
	for (U32 row = 0; row < 4; ++row)
	{
		for (U32 col = 0; col < 4; ++col)
		{
			cofactorMat[row][col] = CalcCofactor(*this, row, col);
		}
	}

	// Turn cofactor matrix into adjoint matrix by transposing
	cofactorMat.Transpose();

	*this = cofactorMat * invDeterminant;
};

inline Mat44 Mat44::Inversed() const
{
	Mat44 copy = *this;
	copy.Inverse();
	return copy;
}

// Inverses rotation and translation
inline void Mat44::FastOrthoInverse()
{
	// Transpose upper 3x3 matrix
	Swap(data[1], data[4]);
	Swap(data[2], data[8]);
	Swap(data[6], data[9]);

	// Negate translation
	t.xyz = -t.xyz;
}

inline Mat44 Mat44::FastOrthoInversed() const
{
	Mat44 copy = *this;
	copy.FastOrthoInverse();
	return copy;
}

inline void Mat44::Scale(F32 uniformScale)
{
	i.xyz *= uniformScale;
	j.xyz *= uniformScale;
	k.xyz *= uniformScale;
}

inline void Mat44::Scale(F32 iScale, F32 jScale, F32 kScale)
{
	i.xyz *= iScale;
	j.xyz *= jScale;
	k.xyz *= kScale;
}

inline void Mat44::Scale(const Vec3& ijkScale)
{
	i.xyz *= ijkScale.x;
	j.xyz *= ijkScale.y;
	k.xyz *= ijkScale.z;
}

inline Mat44 Mat44::Scaled(F32 uniformScale) const
{
	Mat44 copy = *this;
	copy.Scale(uniformScale);
	return copy;
}

inline Mat44 Mat44::Scaled(F32 iScale, F32 jScale, F32 kScale) const
{
	Mat44 copy = *this;
	copy.Scale(iScale, jScale, kScale);
	return copy;
}

inline Mat44 Mat44::Scaled(const Vec3& ijkScale) const
{
	Mat44 copy = *this;
	copy.Scale(ijkScale);
	return copy;
}

inline void Mat44::SetTranslation(const Vec3& inT)
{
	t.xyz = inT;
}

inline void Mat44::Translate(const Vec3& inT)
{
	SetTranslation(t.xyz + inT);
}

inline Mat44 Mat44::CreateTranslation(const Vec3& t)
{
	Mat44 translation = Mat44::IDENTITY;
	translation.SetTranslation(t);
	return translation;
}

inline void Mat44::RotateXRads(F32 xRads)
{
	F32 cosRads = cosf(xRads);
	F32 sinRads = sinf(xRads);

	Mat44 xRot = Mat44(1.0f, 0.0f, 0.0f, 0.0f,
					   0.0f, cosRads, sinRads, 0.0f,
					   0.0f, -sinRads, cosRads, 0.0f,
					   0.0f, 0.0f, 0.0f, 1.0f);

	*this *= xRot;
}

inline void Mat44::RotateYRads(F32 yRads)
{
	F32 cosRads = cosf(yRads);
	F32 sinRads = sinf(yRads);

	Mat44 yRot = Mat44(cosRads, 0.0f, -sinRads, 0.0f,
					   0.0f, 1.0f, 0.0f, 0.0f,
					   sinRads, 0.0f, cosRads, 0.0f,
					   0.0f, 0.0f, 0.0f, 1.0f);

	*this *= yRot;
}

inline void Mat44::RotateZRads(F32 zRads)
{
	F32 cosRads = cosf(zRads);
	F32 sinRads = sinf(zRads);

	Mat44 zRot = Mat44(cosRads, sinRads, 0.0f, 0.0f,
					   -sinRads, cosRads, 0.0f, 0.0f,
					   0.0f, 0.0f, 1.0f, 0.0f,
					   0.0f, 0.0f, 0.0f, 1.0f);

	*this *= zRot;
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

inline Mat44 Mat44::CreateRotationXRads(F32 xRads)
{
	Mat44 m = Mat44::IDENTITY;
	m.RotateYRads(xRads);
	return m;
}

inline Mat44 Mat44::CreateRotationYRads(F32 yRads)
{
	Mat44 m = Mat44::IDENTITY;
	m.RotateYRads(yRads);
	return m;
}

inline Mat44 Mat44::CreateRotationZRads(F32 zRads)
{
	Mat44 m = Mat44::IDENTITY;
	m.RotateZRads(zRads);
	return m;
}

inline Mat44 Mat44::CreateRotationXDegs(F32 xDegs)
{
	Mat44 m = Mat44::IDENTITY;
	m.RotateXDegs(xDegs);
	return m;
}

inline Mat44 Mat44::CreateRotationYDegs(F32 yDegs)
{
	Mat44 m = Mat44::IDENTITY;
	m.RotateYDegs(yDegs);
	return m;
}

inline Mat44 Mat44::CreateRotationZDegs(F32 zDegs)
{
	Mat44 m = Mat44::IDENTITY;
	m.RotateZDegs(zDegs);
	return m;
}

inline void Mat44::RotateAroundAxisRads(const Vec3& axis, F32 rads)
{
	Vec3 iBasis = axis.Normalized();
	Vec3 jBasis = Cross(Vec3::UP, axis).Normalized();
	Vec3 kBasis = Cross(iBasis, jBasis);
	Mat44 axisOfRotationWorldBasis = Mat44(iBasis, jBasis, kBasis);
	Mat44 axisOfRotationLocalBasis = axisOfRotationWorldBasis.Transposed();
	Mat44 localRotation = Mat44::CreateRotationXRads(rads);

	*this *= (axisOfRotationLocalBasis * localRotation * axisOfRotationWorldBasis);
}

inline void Mat44::RotateAroundAxisDegs(const Vec3& axis, F32 degs)
{
	RotateAroundAxisRads(axis, DegToRad(degs));
}

inline Mat44 Mat44::CreateRotationArounAxisRads(const Vec3& axis, F32 rads)
{
	Mat44 m = Mat44::IDENTITY;
	m.RotateAroundAxisRads(axis, rads);
	return m;
}

inline Mat44 Mat44::CreateRotationAroundAxisDegs(const Vec3& axis, F32 degs)
{
	Mat44 m = Mat44::IDENTITY;
	m.RotateAroundAxisDegs(axis, degs);
	return m;
}

inline Mat44 Mat44::GetRotation() const
{
	Mat44 rot = Mat44::IDENTITY;
	rot.i.xyz = i.xyz;
	rot.j.xyz = j.xyz;
	rot.k.xyz = k.xyz;
	return rot;
}

inline void Mat44::SetRotation(const Mat44& rotation)
{
	i.xyz = rotation.i.xyz;
	j.xyz = rotation.j.xyz;
	k.xyz = rotation.k.xyz;
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

inline Vec3 Mat44::TransformVec(const Vec3& v) const
{
	Vec4 res = Vec4(v, 0.0f) * *this;
	return res.xyz;
}

inline Vec3 Mat44::TransformPoint(const Vec3& p) const
{
	Vec4 res = Vec4(p, 1.0f) * *this;
	return res.xyz;
}

inline void Mat44::TransformInModelSpace(const Mat44& transform)
{
	Vec3 translation = t.xyz;
	*this *= Mat44::CreateTranslation(-translation);
	*this *= transform;
	*this *= Mat44::CreateTranslation(translation);
}

inline Mat44 Mat44::GetTransformedInModelSpace(const Mat44& transform) const
{
	Mat44 copy = *this;
	copy.TransformInModelSpace(transform);
	return copy;
}

inline Mat44 Mat44::CreatePerspectiveProjection(F32 verticalFovDeg, F32 nearPlane, F32 farPlane, F32 aspect)
{
	F32 focalLen = 1.0f / TanDeg(verticalFovDeg * 0.5f);

	return Mat44(focalLen / aspect, 0.0f, 0.0f, 0.0f,
				 0.0f, -focalLen, 0.0f, 0.0f,
				 0.0f, 0.0f, farPlane / (farPlane - nearPlane), 1.0f,
				 0.0f, 0.0f, -nearPlane * farPlane / (farPlane - nearPlane), 0.0f);
}

// TODO: orthographic projection

inline Mat44 Mat44::CreateBasisFromI(const Vec3& inI)
{
	Vec3 iNorm = inI.Normalized();

	// account for iNorm pointing directly up or down when trying to build basis
	Vec3 jNorm = Vec3::ZERO;
	F32 cosAngleAbs = abs(Dot(iNorm, Vec3::UP));
	if (AlmostEquals(cosAngleAbs, 1.0f))
	{
		jNorm = Cross(iNorm, Vec3::FORWARD).Normalized();
	}
	else
	{
		jNorm = Cross(Vec3::UP, iNorm).Normalized();
	}

	Vec3 kNorm = Cross(iNorm, jNorm);

	return Mat44(iNorm, jNorm, kNorm);
}

inline Mat44 Mat44::CreateBasisFromJ(const Vec3& inJ)
{
	Vec3 jNorm = inJ.Normalized();

	// account for j_norm pointing directly up or down when trying to build basis
	Vec3 iNorm = Vec3::ZERO;
	F32 cosAngleAbs = abs(Dot(jNorm, Vec3::UP));
	if (AlmostEquals(cosAngleAbs, 1.0f))
	{
		iNorm = Cross(Vec3::LEFT, jNorm).Normalized();
	}
	else
	{
		iNorm = Cross(jNorm, Vec3::UP).Normalized();
	}

	Vec3 kNorm = Cross(iNorm, jNorm);

	return Mat44(iNorm, jNorm, kNorm);
}

inline Mat44 Mat44::CreateBasisFromK(const Vec3& inK)
{
	Vec3 kNorm = inK.Normalized();

	// account for k_norm pointing directly up or down when trying to build basis
	Vec3 iNorm = Vec3::ZERO;
	F32 cosAngleAbs = abs(Dot(kNorm, Vec3::UP));
	if (AlmostEquals(cosAngleAbs, 1.0f))
	{
		iNorm = Cross(Vec3::FORWARD, kNorm).Normalized();
	}
	else
	{
		iNorm = Cross(kNorm, Vec3::UP).Normalized();
	}

	Vec3 jNorm = Cross(kNorm, iNorm);

	return Mat44(iNorm, jNorm, kNorm);
}

inline Mat44 operator*(F32 s, const Mat44& m)
{
	return m * s;
}
