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

	static Mat44 CreateBasisFromI(const Vec3& inI);
	static Mat44 CreateBasisFromJ(const Vec3& inJ);
	static Mat44 CreateBasisFromK(const Vec3& inK);

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

//inline
//mat44& operator*=(mat44& m, f32 s)
//{
//	for (int i = 0; i < 16; i++)
//	{
//		m.data[i] *= s;
//	}
//
//	return m;
//}
//
//inline
//mat44& operator*=(mat44& m, const mat44& other)
//{
//	mat44 result;
//
//	result.ix = (m.ix * other.ix) + (m.iy * other.jx) + (m.iz * other.kx) + (m.iw * other.tx);
//	result.iy = (m.ix * other.iy) + (m.iy * other.jy) + (m.iz * other.ky) + (m.iw * other.ty);
//	result.iz = (m.ix * other.iz) + (m.iy * other.jz) + (m.iz * other.kz) + (m.iw * other.tz);
//	result.iw = (m.ix * other.iw) + (m.iy * other.jw) + (m.iz * other.kw) + (m.iw * other.tw);
//
//	result.jx = (m.jx * other.ix) + (m.jy * other.jx) + (m.jz * other.kx) + (m.jw * other.tx);
//	result.jy = (m.jx * other.iy) + (m.jy * other.jy) + (m.jz * other.ky) + (m.jw * other.ty);
//	result.jz = (m.jx * other.iz) + (m.jy * other.jz) + (m.jz * other.kz) + (m.jw * other.tz);
//	result.jw = (m.jx * other.iw) + (m.jy * other.jw) + (m.jz * other.kw) + (m.jw * other.tw);
//
//	result.kx = (m.kx * other.ix) + (m.ky * other.jx) + (m.kz * other.kx) + (m.kw * other.tx);
//	result.ky = (m.kx * other.iy) + (m.ky * other.jy) + (m.kz * other.ky) + (m.kw * other.ty);
//	result.kz = (m.kx * other.iz) + (m.ky * other.jz) + (m.kz * other.kz) + (m.kw * other.tz);
//	result.kw = (m.kx * other.iw) + (m.ky * other.jw) + (m.kz * other.kw) + (m.kw * other.tw);
//
//	result.tx = (m.tx * other.ix) + (m.ty * other.jx) + (m.tz * other.kx) + (m.tw * other.tx);
//	result.ty = (m.tx * other.iy) + (m.ty * other.jy) + (m.tz * other.ky) + (m.tw * other.ty);
//	result.tz = (m.tx * other.iz) + (m.ty * other.jz) + (m.tz * other.kz) + (m.tw * other.tz);
//	result.tw = (m.tx * other.iw) + (m.ty * other.jw) + (m.tz * other.kw) + (m.tw * other.tw);
//
//	m = result;
//	return m;
//}
//
//inline
//mat44 operator*(const mat44& a, const mat44& b)
//{
//	mat44 copy = a;
//	copy *= b;
//	return copy;
//}
//
//inline
//mat44 operator*(f32 s, const mat44& m)
//{
//	return m * s;
//}
//
//inline
//void transpose(mat44& m)
//{
//	swap(m.data[1], m.data[4]);
//	swap(m.data[2], m.data[8]);
//	swap(m.data[3], m.data[12]);
//	swap(m.data[6], m.data[9]);
//	swap(m.data[7], m.data[13]);
//	swap(m.data[11], m.data[14]);
//}
//
//inline
//mat44 get_transposed(const mat44& m)
//{
//	mat44 copy = m;
//	transpose(copy);
//	return copy;
//}
//
//inline
//f32 calc_cofactor(const mat44& m, u32 row, u32 column)
//{
//	mat33 submatrix;
//
//	// Build Mat33 submatrix by removing the row and column we are getting cofactor for
//	u32 cur_row = 0;
//	for (u32 src_row = 0; src_row < 4; ++src_row)
//	{
//		if (src_row == row)
//		{
//			continue;
//		}
//
//		u32 cur_col = 0;
//		for (u32 src_col = 0; src_col < 4; ++src_col)
//		{
//			if (src_col == column)
//			{
//				continue;
//			}
//
//			//submatrix.SetElement(currentIndex, GetElement(srcRow, srcCol));
//			submatrix[cur_row][cur_col] = m[src_row][src_col];
//
//			cur_col++;
//		}
//
//		cur_row++;
//	}
//
//	// Calculate determinant of submatrix
//	f32 sub_det = calc_determinant(submatrix);
//
//	// Use correct sign based on row and column
//	if ((row + column) % 2 != 0)
//	{
//		sub_det = -sub_det;
//	}
//
//	return sub_det;
//}
//
//inline
//f32 calc_determinant(const mat44& m)
//{
//	return m.ix * calc_cofactor(m, 0, 0) +
//		m.iy * calc_cofactor(m, 0, 1) +
//		m.iz * calc_cofactor(m, 0, 2) +
//		m.iw * calc_cofactor(m, 0, 3);
//}
//
//inline
//void inverse(mat44& m)
//{
//	// Cramer's method of inverse determinant multiplied by adjoint matrix
//	f32 inv_det = 1.0f / calc_determinant(m);
//
//	// Build matrix of cofactors
//	mat44 cofactor_mat;
//	for (u32 row = 0; row < 4; ++row)
//	{
//		for (u32 col = 0; col < 4; ++col)
//		{
//			cofactor_mat[row][col] = calc_cofactor(m, row, col);
//		}
//	}
//
//	// Turn cofactor matrix into adjoint matrix by transposing
//	transpose(cofactor_mat);
//
//	m = cofactor_mat * inv_det;
//};
//
//inline
//mat44 get_inversed(const mat44& m)
//{
//	mat44 copy = m;
//	inverse(copy);
//	return copy;
//}
//
//// Inverses rotation and translation
//inline
//void fast_ortho_inverse(mat44& m)
//{
//	// Transpose upper 3x3 matrix
//	swap(m.data[1], m.data[4]);
//	swap(m.data[2], m.data[8]);
//	swap(m.data[6], m.data[9]);
//
//	// Negate translation
//	m.t.xyz = -m.t.xyz;
//}
//
//inline
//mat44 get_fast_ortho_inversed(const mat44& m)
//{
//	mat44 copy = m;
//	fast_ortho_inverse(copy);
//	return copy;
//}
//
//inline
//void scale(mat44& m, f32 uniform_scale)
//{
//	m.i.xyz *= uniform_scale;
//	m.j.xyz *= uniform_scale;
//	m.k.xyz *= uniform_scale;
//}
//
//inline
//void scale(mat44& m, f32 i_scale, f32 j_scale, f32 k_scale)
//{
//	m.i.x *= i_scale;
//	m.i.y *= j_scale;
//	m.i.z *= k_scale;
//
//	m.j.x *= i_scale;
//	m.j.y *= j_scale;
//	m.j.z *= k_scale;
//
//	m.k.x *= i_scale;
//	m.k.y *= j_scale;
//	m.k.z *= k_scale;
//}
//
//inline
//void scale(mat44& m, const vec3& ijk_scale_factors)
//{
//	m.i.x *= ijk_scale_factors.x;
//	m.i.y *= ijk_scale_factors.y;
//	m.i.z *= ijk_scale_factors.z;
//
//	m.j.x *= ijk_scale_factors.x;
//	m.j.y *= ijk_scale_factors.y;
//	m.j.z *= ijk_scale_factors.z;
//
//	m.k.x *= ijk_scale_factors.x;
//	m.k.y *= ijk_scale_factors.y;
//	m.k.z *= ijk_scale_factors.z;
//}
//
//inline
//mat44 get_scaled(mat44& m, f32 uniform_scale)
//{
//	mat44 copy = m;
//	scale(copy, uniform_scale);
//	return copy;
//}
//
//inline
//mat44 get_scaled(mat44& m, f32 i_scale, f32 j_scale, f32 k_scale)
//{
//	mat44 copy = m;
//	scale(m, i_scale, j_scale, k_scale);
//	return copy;
//}
//
//inline
//mat44 get_scaled(const mat44& m, const vec3& ijk_scale_factors)
//{
//	mat44 copy = m;
//	scale(copy, ijk_scale_factors);
//	return copy;
//}
//
//inline
//void scale_local(mat44& m, f32 uniform_scale)
//{
//	m.i.xyz *= uniform_scale;
//	m.j.xyz *= uniform_scale;
//	m.k.xyz *= uniform_scale;
//}
//
//inline
//void scale_local(mat44& m, f32 i_scale, f32 j_scale, f32 k_scale)
//{
//	m.i *= i_scale;
//	m.j *= j_scale;
//	m.k *= k_scale;
//}
//
//inline
//void scale_local(mat44& m, const vec3& ijk_scale_factors)
//{
//	m.i *= ijk_scale_factors.x;
//	m.j *= ijk_scale_factors.y;
//	m.k *= ijk_scale_factors.z;
//}
//
//inline
//mat44 get_scaled_local(mat44& m, f32 uniform_scale)
//{
//	mat44 copy = m;
//	scale_local(copy, uniform_scale);
//	return copy;
//}
//
//inline
//mat44 get_scaled_local(mat44& m, f32 i_scale, f32 j_scale, f32 k_scale)
//{
//	mat44 copy = m;
//	scale_local(m, i_scale, j_scale, k_scale);
//	return copy;
//}
//
//inline
//mat44 get_scaled_local(const mat44& m, const vec3& ijk_scale_factors)
//{
//	mat44 copy = m;
//	scale_local(copy, ijk_scale_factors);
//	return copy;
//}
//inline
//void set_translation(mat44& m, const vec3& t)
//{
//	m.t = make_vec4(t, 1.0f);
//}
//
//inline
//void translate(mat44& m, const vec3& t)
//{
//	set_translation(m, m.t.xyz + t);
//}
//
//inline
//mat44 make_translation(const vec3& t)
//{
//	mat44 translation = MAT44_IDENTITY;
//	translate(translation, t);
//	return translation;
//}
//
//inline
//void rotate_x_rad(mat44& m, f32 x_rads)
//{
//	f32 cos_rads = cosf(x_rads);
//	f32 sin_rads = sinf(x_rads);
//
//	mat44 x_rot = make_mat44(1.0f, 0.0f, 0.0f, 0.0f,
//		0.0f, cos_rads, sin_rads, 0.0f,
//		0.0f, -sin_rads, cos_rads, 0.0f,
//		0.0f, 0.0f, 0.0f, 1.0f);
//
//	m *= x_rot;
//}
//
//inline
//void rotate_y_rad(mat44& m, f32 y_rads)
//{
//	f32 cos_rads = cosf(y_rads);
//	f32 sin_rads = sinf(y_rads);
//
//	mat44 y_rot = make_mat44(cos_rads, 0.0f, -sin_rads, 0.0f,
//		0.0f, 1.0f, 0.0f, 0.0f,
//		sin_rads, 0.0f, cos_rads, 0.0f,
//		0.0f, 0.0f, 0.0f, 1.0f);
//
//	m *= y_rot;
//}
//
//inline
//void rotate_z_rad(mat44& m, f32 z_rads)
//{
//	f32 cos_rads = cosf(z_rads);
//	f32 sin_rads = sinf(z_rads);
//
//	mat44 z_rot = make_mat44(cos_rads, sin_rads, 0.0f, 0.0f,
//		-sin_rads, cos_rads, 0.0f, 0.0f,
//		0.0f, 0.0f, 1.0f, 0.0f,
//		0.0f, 0.0f, 0.0f, 1.0f);
//
//	m *= z_rot;
//}
//
//inline
//void rotate_x_deg(mat44& m, f32 x_deg)
//{
//	rotate_x_rad(m, deg_to_rad(x_deg));
//}
//
//inline
//void rotate_y_deg(mat44& m, f32 y_deg)
//{
//	rotate_y_rad(m, deg_to_rad(y_deg));
//}
//
//inline
//void rotate_z_deg(mat44& m, f32 z_deg)
//{
//	rotate_z_rad(m, deg_to_rad(z_deg));
//}
//
//inline
//mat44 make_rotation_x_deg(f32 x_deg)
//{
//	mat44 m = MAT44_IDENTITY;
//	rotate_x_deg(m, x_deg);
//	return m;
//}
//
//inline
//mat44 make_rotation_y_deg(f32 y_deg)
//{
//	mat44 m = MAT44_IDENTITY;
//	rotate_y_deg(m, y_deg);
//	return m;
//}
//
//inline
//mat44 make_rotation_z_deg(f32 z_deg)
//{
//	mat44 m = MAT44_IDENTITY;
//	rotate_z_deg(m, z_deg);
//	return m;
//}
//
//inline
//mat44 make_rotation_x_rad(f32 x_rad)
//{
//	mat44 m = MAT44_IDENTITY;
//	rotate_x_rad(m, x_rad);
//	return m;
//}
//
//inline
//mat44 make_rotation_y_rad(f32 y_rad)
//{
//	mat44 m = MAT44_IDENTITY;
//	rotate_y_rad(m, y_rad);
//	return m;
//}
//
//inline
//mat44 make_rotation_z_rad(f32 z_rad)
//{
//	mat44 m = MAT44_IDENTITY;
//	rotate_z_rad(m, z_rad);
//	return m;
//}
//
//inline
//void rotate_around_axis_rad(mat44& m, const vec3& axis, f32 rad)
//{
//	vec3 i = get_normalized(axis);
//	vec3 j = get_normalized(cross(VEC3_UP, axis));
//	vec3 k = cross(i, j);
//	mat44 axis_rotation_world_basis = make_mat44(i, j, k);
//	mat44 change_of_basis = get_transposed(axis_rotation_world_basis);
//	mat44 local_rotation = make_rotation_x_rad(rad);
//
//	m *= (change_of_basis * local_rotation * axis_rotation_world_basis);
//}
//
//inline
//void rotate_around_axis_deg(mat44& m, const vec3& axis, f32 deg)
//{
//	rotate_around_axis_rad(m, axis, deg_to_rad(deg));
//}
//
//inline
//mat44 make_rotation_around_axis_rad(const vec3& axis, f32 rad)
//{
//	mat44 m = MAT44_IDENTITY;
//	rotate_around_axis_rad(m, axis, rad);
//	return m;
//}
//
//inline
//mat44 make_rotation_around_axis_deg(const vec3& axis, f32 deg)
//{
//	mat44 m = MAT44_IDENTITY;
//	rotate_around_axis_deg(m, axis, deg);
//	return m;
//}
//
//inline
//mat44 get_rotation(mat44& m)
//{
//	mat44 rot_only = MAT44_IDENTITY;
//	rot_only.i.xyz = m.i.xyz;
//	rot_only.j.xyz = m.j.xyz;
//	rot_only.k.xyz = m.k.xyz;
//	return rot_only;
//}
//
//inline
//void set_rotation(mat44& m, const mat44& rotation)
//{
//	m.i.xyz = rotation.i.xyz;
//	m.j.xyz = rotation.j.xyz;
//	m.k.xyz = rotation.k.xyz;
//}
//
//inline
//vec4 operator*(const vec4& v, const mat44& mat)
//{
//	vec4 result;
//	result.x = (v.x * mat.ix) + (v.y * mat.jx) + (v.z * mat.kx) + (v.w * mat.tx);
//	result.y = (v.x * mat.iy) + (v.y * mat.jy) + (v.z * mat.ky) + (v.w * mat.ty);
//	result.z = (v.x * mat.iz) + (v.y * mat.jz) + (v.z * mat.kz) + (v.w * mat.tz);
//	result.w = (v.x * mat.iw) + (v.y * mat.jw) + (v.z * mat.kw) + (v.w * mat.tw);
//	return result;
//}
//
//inline
//vec3 transform_vec(const mat44& m, const vec3& v)
//{
//	vec4 res = make_vec4(v, 0.0f) * m;
//	return res.xyz;
//}
//
//inline
//vec3 transform_point(const mat44& m, const vec3& p)
//{
//	vec4 res = make_vec4(p, 1.0f) * m;
//	return res.xyz;
//}
//
//inline
//void transform_in_model_space(mat44& m, const mat44& transform)
//{
//	vec3 translation = m.t.xyz;
//	m *= make_translation(-translation);
//	m *= transform;
//	m *= make_translation(translation);
//}
//
//inline
//mat44 get_transformed_in_model_space(const mat44& m, const mat44& transform)
//{
//	mat44 copy = m;
//	transform_in_model_space(copy, transform);
//	return copy;
//}
//
//inline
//mat44 create_perspective_projection(f32 vertical_fov_deg, f32 near_plane, f32 far_plane, f32 aspect)
//{
//	f32 focal_len = 1.0f / tan_deg(vertical_fov_deg * 0.5f);
//
//	return make_mat44(focal_len / aspect, 0.0f, 0.0f, 0.0f,
//		0.0f, -focal_len, 0.0f, 0.0f,
//		0.0f, 0.0f, far_plane / (far_plane - near_plane), 1.0f,
//		0.0f, 0.0f, -near_plane * far_plane / (far_plane - near_plane), 0.0f);
//}
//
//// TODO(smerendino): orthographic projection
