#pragma once
#pragma warning(disable:4201)

#include "engine/math/vec3.h"
#include "engine/core/funcs.h"

#include <memory.h>

struct mat33
{
	inline mat33(){};

    f32* operator[](u32 row);
    const f32* operator[](u32 row) const;

    union
    {
        struct
        {
            vec3 i;
            vec3 j;
            vec3 k;
        };

        struct
        {
			f32 ix, iy, iz;
			f32 jx, jy, jz;
			f32 kx, ky, kz;
        };

        f32 data[9];
    };
};

inline 
mat33 make_mat33(const vec3& i, const vec3& j, const vec3& k)
{
    mat33 m;
    m.i = i;
    m.j = j;
    m.k = k;
    return m;
}

inline 
mat33 make_mat33(f32 ix, f32 iy, f32 iz, 
                 f32 jx, f32 jy, f32 jz,
                 f32 kx, f32 ky, f32 kz)
{
    mat33 m;
    m.i = make_vec3(ix, iy, iz);
    m.j = make_vec3(jx, jy, jz);
    m.k = make_vec3(kx, ky, kz);
    return m;
}

inline 
mat33 make_mat33(f32* data)
{
	ASSERT(nullptr != data);
    mat33 m;
	memcpy(m.data, data, sizeof(mat33));
    return m;
}

static const mat33 MAT33_IDENTITY = make_mat33(1.0f, 0.0f, 0.0f,
							                   0.0f, 1.0f, 0.0f,
							                   0.0f, 0.0f, 1.0f);

inline
f32* mat33::operator[](u32 row)
{
    ASSERT(row >= 0 && row <= 2); 
    return &data[row * 3]; 
}

inline
const f32* mat33::operator[](u32 row) const
{
    ASSERT(row >= 0 && row <= 2); 
    return &data[row * 3]; 
}

inline 
mat33 operator*(const mat33& m, f32 s)
{
    return make_mat33(m.i * s, m.j * s, m.k * s);
}

inline 
mat33& operator*=(mat33& m, f32 s)
{
    for (int i = 0; i < 9; i++)
    {
        m.data[i] *= s;
    }

    return m;
}

inline
mat33& operator*=(mat33& m, const mat33& other)
{
	mat33 result;

	result.ix = (m.ix * other.ix) + (m.iy * other.jx) + (m.iz * other.kx);
	result.iy = (m.ix * other.iy) + (m.iy * other.jy) + (m.iz * other.ky);
	result.iz = (m.ix * other.iz) + (m.iy * other.jz) + (m.iz * other.kz);

	result.jx = (m.jx * other.ix) + (m.jy * other.jx) + (m.jz * other.kx);
	result.jy = (m.jx * other.iy) + (m.jy * other.jy) + (m.jz * other.ky);
	result.jz = (m.jx * other.iz) + (m.jy * other.jz) + (m.jz * other.kz);

	result.kx = (m.kx * other.ix) + (m.ky * other.jx) + (m.kz * other.kx);
	result.ky = (m.kx * other.iy) + (m.ky * other.jy) + (m.kz * other.ky);
	result.kz = (m.kx * other.iz) + (m.ky * other.jz) + (m.kz * other.kz);

	m = result;
	return m;
}

inline 
mat33 operator*(const mat33& a, const mat33& b) 
{
	mat33 copy = a;
	copy *= b;
	return copy;
}

inline
void transpose(mat33& m)
{
	swap(m.data[1], m.data[3]);
	swap(m.data[2], m.data[6]);
	swap(m.data[5], m.data[7]);
}

inline 
mat33 get_transposed(const mat33& m) 
{
	mat33 copy = m;
    transpose(copy);
    return copy;
}

inline
mat33 operator*(f32 s,  mat33& m)
{
	return m * s;
}

inline 
vec3 operator*(const vec3& v, const mat33& mat)
{
	vec3 result;
	result.x = (v.x * mat.ix) + (v.y * mat.jx) + (v.z * mat.kx);
	result.y = (v.x * mat.iy) + (v.y * mat.jy) + (v.z * mat.ky);
	result.z = (v.x * mat.iz) + (v.y * mat.jz) + (v.z * mat.kz);
	return result;
}

inline
f32 calc_determinant(const mat33& m)
{ 
	return m.ix * (m.jy * m.kz - m.jz * m.ky) -
		   m.iy * (m.jx * m.kz - m.jz * m.kx) +
		   m.iz * (m.jx * m.ky - m.jy * m.kx);
}
