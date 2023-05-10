#pragma once
#pragma warning(disable:4201)

#include "engine/core/funcs.h"
#include "engine/math/mat33.h"
#include "engine/math/vec4.h"
#include "engine/math/vec3.h"

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

struct mat44
{
	inline mat44(){};
	inline mat44(const vec4& i, const vec4& j, const vec4& k, const vec4& t);
	inline mat44(f32 ix, f32 iy, f32 iz, f32 iw,
				 f32 jx, f32 jy, f32 jz, f32 jw,
				 f32 kx, f32 ky, f32 kz, f32 kw,
				 f32 tx, f32 ty, f32 tz, f32 tw);
	inline mat44(f32 *data);

    inline f32* operator[](u32 row);
    inline const f32* operator[](u32 row) const;

    union
    {
        struct
        {
			vec4 i;
			vec4 j;
			vec4 k;
			vec4 t;
        };

        struct
        {
			f32 ix, iy, iz, iw;
			f32 jx, jy, jz, jw;
			f32 kx, ky, kz, kw;
			f32 tx, ty, tz, tw;
        };

        f32 data[16];
    };
};

mat44 MAT44_IDENTITY = mat44(1.0f, 0.0f, 0.0f, 0.0f,
							 0.0f, 1.0f, 0.0f, 0.0f,
							 0.0f, 0.0f, 1.0f, 0.0f,
							 0.0f, 0.0f, 0.0f, 1.0f);

inline 
mat44::mat44(const vec4& in_i, const vec4& in_j, const vec4& in_k, const vec4& in_t)
    :i(in_i)
    ,j(in_j)
    ,k(in_k)
    ,t(in_t)
{
}

inline 
mat44::mat44(f32 ix, f32 iy, f32 iz, f32 iw,
             f32 jx, f32 jy, f32 jz, f32 jw,
             f32 kx, f32 ky, f32 kz, f32 kw,
             f32 tx, f32 ty, f32 tz, f32 tw)
    :i(ix, iy, iz, iw)
    ,j(jx, jy, jz, jw)
    ,k(kx, ky, kz, kw)
    ,t(tx, ty, tz, tw)
{
}

inline 
mat44::mat44(f32* in_data)
{
	ASSERT(nullptr != in_data);
	memcpy(data, in_data, sizeof(mat44));
}

inline
f32* mat44::operator[](u32 row)
{
    ASSERT(row >= 0 && row <= 3);
    return &data[row * 4];
}

inline
const f32* mat44::operator[](u32 row) const
{
    ASSERT(row >= 0 && row <= 3);
    return &data[row * 4];
}

inline 
mat44 operator*(const mat44& m, f32 s)
{
    return mat44(m.i * s, m.j * s, m.k * s, m.t * s);
}

inline 
mat44& operator*=(mat44& m, f32 s)
{
	for (int i = 0; i < 16; i++)
	{
		m.data[i] *= s;
	}

	return m;
}

inline
mat44& operator*=(mat44& m, const mat44& other)
{
	mat44 result;

	result.ix = (m.ix * other.ix) + (m.iy * other.jx) + (m.iz * other.kx) + (m.iw * other.tx);
	result.iy = (m.ix * other.iy) + (m.iy * other.jy) + (m.iz * other.ky) + (m.iw * other.ty);
	result.iz = (m.ix * other.iz) + (m.iy * other.jz) + (m.iz * other.kz) + (m.iw * other.tz);
	result.iw = (m.ix * other.iw) + (m.iy * other.jw) + (m.iz * other.kw) + (m.iw * other.tw);

	result.jx = (m.jx * other.ix) + (m.jy * other.jx) + (m.jz * other.kx) + (m.jw * other.tx);
	result.jy = (m.jx * other.iy) + (m.jy * other.jy) + (m.jz * other.ky) + (m.jw * other.ty);
	result.jz = (m.jx * other.iz) + (m.jy * other.jz) + (m.jz * other.kz) + (m.jw * other.tz);
	result.jw = (m.jx * other.iw) + (m.jy * other.jw) + (m.jz * other.kw) + (m.jw * other.tw);

	result.kx = (m.kx * other.ix) + (m.ky * other.jx) + (m.kz * other.kx) + (m.kw * other.tx);
	result.ky = (m.kx * other.iy) + (m.ky * other.jy) + (m.kz * other.ky) + (m.kw * other.ty);
	result.kz = (m.kx * other.iz) + (m.ky * other.jz) + (m.kz * other.kz) + (m.kw * other.tz);
	result.kw = (m.kx * other.iw) + (m.ky * other.jw) + (m.kz * other.kw) + (m.kw * other.tw);

	result.tx = (m.tx * other.ix) + (m.ty * other.jx) + (m.tz * other.kx) + (m.tw * other.tx);
	result.ty = (m.tx * other.iy) + (m.ty * other.jy) + (m.tz * other.ky) + (m.tw * other.ty);
	result.tz = (m.tx * other.iz) + (m.ty * other.jz) + (m.tz * other.kz) + (m.tw * other.tz);
	result.tw = (m.tx * other.iw) + (m.ty * other.jw) + (m.tz * other.kw) + (m.tw * other.tw);

	m = result;
	return m;
}

inline 
mat44 operator*(const mat44& a, const mat44& b)
{
	mat44 copy = a;
	copy *= b;
	return copy;
}

inline
void transpose(mat44& m)
{
	swap(m.data[1], m.data[4]);
	swap(m.data[2], m.data[8]);
	swap(m.data[3], m.data[12]);
	swap(m.data[6], m.data[9]);
	swap(m.data[7], m.data[13]);
	swap(m.data[11], m.data[14]);
}

inline 
mat44 transposed(const mat44& m)
{
	mat44 copy = m;
    transpose(copy);
    return copy;
}

inline
f32 calc_cofactor(const mat44& m, u32 row, u32 column)
{
	mat33 submatrix;
	
	// Build Mat33 submatrix by removing the row and column we are getting cofactor for
	u32 cur_row = 0;
	u32 cur_col = 0;
	for (u32 src_row = 0; src_row < 4; ++src_row)
	{
		if (src_row == row)
		{
			continue;
		}

		for (u32 src_col = 0; src_col < 4; ++src_col)
		{
			if (src_col == column)
			{
				continue;
			}

			//submatrix.SetElement(currentIndex, GetElement(srcRow, srcCol));
            submatrix[cur_row][cur_col] = m[src_row][src_col];

            cur_col++;
		}

        cur_row++;
	}

	// Calculate determinant of submatrix
	f32 sub_det = calc_determinant(submatrix);

	// Use correct sign based on row and column
	if ((row + column) % 2 != 0)
	{
		sub_det = -sub_det;
	}
	
	return sub_det;
}

inline
f32 calc_determinant(const mat44& m)
{ 
	return m.ix * calc_cofactor(m, 0, 0) +
		   m.iy * calc_cofactor(m, 0, 1) +
		   m.iz * calc_cofactor(m, 0, 2) +
		   m.iw * calc_cofactor(m, 0, 3);
}

inline
void inverse(mat44& m) 
{
	// Cramer's method of inverse determinant multiplied by adjoint matrix
	f32 inv_det = 1.0f / calc_determinant(m);

	// Build matrix of cofactors
	mat44 cofactor_mat;
	for (u32 row = 0; row < 4; ++row)
	{
		for (u32 col = 0; col < 4; ++col)
		{
            cofactor_mat[row][col] = calc_cofactor(m, row, col);
		}
	}

	// Turn cofactor matrix into adjoint matrix by transposing
    transpose(cofactor_mat);

	m = cofactor_mat * inv_det;
};

inline 
mat44 get_inversed(const mat44&& m)
{
	mat44 copy = m;
    inverse(copy);
	return copy;
}

// Inverses rotation and translation
inline
void fast_ortho_inverse(mat44& m)
{
	// Transpose upper 3x3 matrix
	swap(m.data[1], m.data[4]);
	swap(m.data[2], m.data[8]);
	swap(m.data[6], m.data[9]);

	// Negate translation
	m.t.xyz = -m.t.xyz;
}

inline 
mat44 get_fast_ortho_inversed(const mat44& m)
{
	mat44 copy = m;
    fast_ortho_inverse(copy);
	return copy;
}

inline 
mat44 GetScaled(f32 uniformScale) const
{
	mat44 scaled = *this;
	scaled.Scale(uniformScale);
	return scaled;
}

inline 
mat44 GetScaled(f32 iScale, f32 jScale, f32 kScale) const
{
	mat44 scaled = *this;
	scaled.Scale(iScale, jScale, kScale);
	return scaled;
}

inline 
mat44 GetScaled(const vec3& scaleFactors) const
{
	mat44 scaled = *this;
	scaled.Scale(scaleFactors);
	return scaled;
}

inline
mat44 operator*(f32 s, const mat44& m)
{
	return m * s;
}

void Mat44::Translate(const Vec3& t)
{
	SetTranslation(m_t.m_xyz + t);
}

void Mat44::SetTranslation(const Vec3& t)
{
	m_t = Vec4(t, 1.0f);
}

Vec3 Mat44::GetTranslation() const
{
	return m_t.m_xyz;
}

Mat44 Mat44::MakeTranslation(const Vec3& t)
{
	Mat44 translation = Mat44::IDENTITY;
	translation.Translate(t);
	return translation;
}

void Mat44::RotateXDeg(F32 xDegrees)
{
	RotateXRad(DegreesToRadians(xDegrees));
}

void Mat44::RotateYDeg(F32 yDegrees)
{
	RotateYRad(DegreesToRadians(yDegrees));
}

void Mat44::RotateZDeg(F32 zDegrees)
{
	RotateZRad(DegreesToRadians(zDegrees));
}

void Mat44::RotateXRad(F32 xRadians)
{
	F32 cosRads = cosf(xRadians);
	F32 sinRads = sinf(xRadians);

	Mat44 xRot = Mat44(1.0f, 0.0f, 0.0f, 0.0f,
					   0.0f, cosRads, sinRads, 0.0f,
					   0.0f, -sinRads, cosRads, 0.0f,
					   0.0f, 0.0f, 0.0f, 1.0f);

	*this *= xRot;
}

void Mat44::RotateYRad(F32 yRadians)
{
	F32 cosRads = cosf(yRadians);
	F32 sinRads = sinf(yRadians);

	Mat44 yRot = Mat44(cosRads, 0.0f, -sinRads, 0.0f,
					   0.0f, 1.0f, 0.0f, 0.0f,
					   sinRads, 0.0f, cosRads, 0.0f,
					   0.0f, 0.0f, 0.0f, 1.0f);

	*this *= yRot;
}

void Mat44::RotateZRad(F32 zRadians)
{
	F32 cosRads = cosf(zRadians);
	F32 sinRads = sinf(zRadians);

	Mat44 zRot = Mat44(cosRads, sinRads, 0.0f, 0.0f,
					   -sinRads, cosRads, 0.0f, 0.0f,
					   0.0f, 0.0f, 1.0f, 0.0f,
					   0.0f, 0.0f, 0.0f, 1.0f);

	*this *= zRot;
}

Mat44 Mat44::MakeRotationXDeg(F32 xDegrees)
{
	Mat44 rotation = Mat44::IDENTITY;
	rotation.RotateXDeg(xDegrees);
	return rotation;
}

Mat44 Mat44::MakeRotationYDeg(F32 yDegrees)
{
	Mat44 rotation = Mat44::IDENTITY;
	rotation.RotateYDeg(yDegrees);
	return rotation;
}

Mat44 Mat44::MakeRotationZDeg(F32 zDegrees)
{
	Mat44 rotation = Mat44::IDENTITY;
	rotation.RotateZDeg(zDegrees);
	return rotation;
}

Mat44 Mat44::MakeRotationXRad(F32 xRadians)
{
	Mat44 rotation = Mat44::IDENTITY;
	rotation.RotateXRad(xRadians);
	return rotation;
}

Mat44 Mat44::MakeRotationYRad(F32 yRadians)
{
	Mat44 rotation = Mat44::IDENTITY;
	rotation.RotateYRad(yRadians);
	return rotation;
}

Mat44 Mat44::MakeRotationZRad(F32 zRadians)
{
	Mat44 rotation = Mat44::IDENTITY;
	rotation.RotateZRad(zRadians);
	return rotation;
}

Mat44 Mat44::GetRotation()
{
	Mat44 rotationOnly = Mat44::IDENTITY;
	rotationOnly.SetIBasis(this->GetIBasis());
	rotationOnly.SetJBasis(this->GetJBasis());
	rotationOnly.SetKBasis(this->GetKBasis());
	return rotationOnly;
}

void Mat44::SetRotation(const Mat44& rotation)
{
	SetIBasis(rotation.GetIBasis());
	SetJBasis(rotation.GetJBasis());
	SetKBasis(rotation.GetKBasis());
}

void Mat44::Scale(F32 uniformScale)
{
	m_i.m_xyz *= uniformScale;
	m_j.m_xyz *= uniformScale;
	m_k.m_xyz *= uniformScale;
}

void Mat44::Scale(F32 iScale, F32 jScale, F32 kScale)
{
	m_i.m_x *= iScale;
	m_i.m_y *= jScale;
	m_i.m_z *= kScale;

	m_j.m_x *= iScale;
	m_j.m_y *= jScale;
	m_j.m_z *= kScale;

	m_k.m_x *= iScale;
	m_k.m_y *= jScale;
	m_k.m_z *= kScale;
}

void Mat44::Scale(const Vec3& scaleFactors)
{
	m_i.m_x *= scaleFactors.m_x;
	m_i.m_y *= scaleFactors.m_y;
	m_i.m_z *= scaleFactors.m_z;

	m_j.m_x *= scaleFactors.m_x;
	m_j.m_y *= scaleFactors.m_y;
	m_j.m_z *= scaleFactors.m_z;

	m_k.m_x *= scaleFactors.m_x;
	m_k.m_y *= scaleFactors.m_y;
	m_k.m_z *= scaleFactors.m_z;
}

void Mat44::SetIBasis(const Vec3& iBasis)
{
	m_i = Vec4(iBasis, 0.0f);
}

void Mat44::SetJBasis(const Vec3& jBasis)
{
	m_j = Vec4(jBasis, 0.0f);
}

void Mat44::SetKBasis(const Vec3& kBasis)
{
	m_k = Vec4(kBasis, 0.0f);
}

Vec3 Mat44::GetIBasis() const
{
	return m_i.m_xyz;
}

Vec3 Mat44::GetJBasis() const
{
	return m_j.m_xyz;
}

Vec3 Mat44::GetKBasis() const
{
	return m_k.m_xyz;
}

Vec3 Mat44::TransformVector(const Vec3& v) const
{
	Vec4 res = Vec4(v, 0.0f) * (*this);
	return res.m_xyz;
}

Vec3 Mat44::TransformPoint(const Vec3& p) const
{
	Vec4 res = Vec4(p, 1.0f) * (*this);
	return res.m_xyz;
}

Vec4 operator*(const Vec4& v, const Mat44& mat)
{
	Vec4 result;
	result.m_x = (v.m_x * mat.m_ix) + (v.m_y * mat.m_jx) + (v.m_z * mat.m_kx) + (v.m_w * mat.m_tx);
	result.m_y = (v.m_x * mat.m_iy) + (v.m_y * mat.m_jy) + (v.m_z * mat.m_ky) + (v.m_w * mat.m_ty);
	result.m_z = (v.m_x * mat.m_iz) + (v.m_y * mat.m_jz) + (v.m_z * mat.m_kz) + (v.m_w * mat.m_tz);
	result.m_w = (v.m_x * mat.m_iw) + (v.m_y * mat.m_jw) + (v.m_z * mat.m_kw) + (v.m_w * mat.m_tw);
	return result;
}

Mat44 CreatePerspectiveProjection(F32 verticalFovDegrees, F32 nearPlane, F32 farPlane, F32 aspect)
{
	F32 focalLength = 1.0f / TanDegrees(verticalFovDegrees * 0.5f);

	return Mat44(focalLength / aspect,	0.0f,			0.0f,								0.0f,
				 0.0f,					-focalLength,	0.0f,								0.0f,
				 0.0f,					0.0f,			farPlane / (farPlane - nearPlane),	1.0f,
				 0.0f,					0.0f,			-nearPlane * farPlane / (farPlane - nearPlane),	0.0f);
}

vec4 operator*(const vec4& v, const mat44& mat);

// perspective projection
mat44 CreatePerspectiveProjection(f32 verticalFoVDegrees, f32 nearPlane, f32 farPlane, f32 aspect);

// orthographic projection

