#pragma once

#include "sm/core/helpers.h"
#include "sm/core/types.h"
#include "sm/math/vec3.h"
#include <cstring>

namespace sm
{
	struct mat33_t
	{
        f32 ix = 1.0f, iy = 0.0f, iz = 0.0f;
        f32 jx = 0.0f, jy = 1.0f, jz = 0.0f;
        f32 kx = 0.0f, ky = 0.0f, kz = 1.0f;

        inline f32* operator[](u32 row);
        inline const f32* operator[](u32 row) const;
        inline mat33_t operator*(f32 s) const;
        inline mat33_t& operator*=(f32 s);
        inline mat33_t operator*(const mat33_t& other) const;
        inline mat33_t& operator*=(const mat33_t& other);
	};

	inline mat33_t  init_mat33(const vec3_t& i, const vec3_t& j, const vec3_t& k);
	inline mat33_t  init_mat33(f32 ix, f32 iy, f32 iz,
                               f32 jx, f32 jy, f32 jz,
                               f32 kx, f32 ky, f32 kz);
	inline mat33_t  init_mat33(f32* data);

    inline vec3_t   get_i_basis(const mat33_t& m) { return vec3_t(m.ix, m.iy, m.iz); }
    inline vec3_t   get_j_basis(const mat33_t& m) { return vec3_t(m.jx, m.jy, m.jz); }
    inline vec3_t   get_k_basis(const mat33_t& m) { return vec3_t(m.kx, m.ky, m.kz); }
    inline void     set_i_basis(mat33_t& m, const vec3_t& i);
    inline void     set_j_basis(mat33_t& m, const vec3_t& j);
    inline void     set_k_basis(mat33_t& m, const vec3_t& k);

	inline void     transpose(mat33_t& m);
	inline mat33_t  transposed(const mat33_t& m);
	inline f32      determinant(const mat33_t& m);

    inline f32* mat33_t::operator[](u32 row)
    {
        SM_ASSERT(row >= 0 && row <= 2);
        f32* row_start = &ix + (row * 3);
        return row_start;
    }

    inline const f32* mat33_t::operator[](u32 row) const
    {
        SM_ASSERT(row >= 0 && row <= 2);
        const f32* row_start = &ix + (row * 3);
        return row_start;
    }

    inline mat33_t mat33_t::operator*(f32 s) const
    {
        return init_mat33(get_i_basis(*this) * s,
                          get_j_basis(*this) * s,
                          get_k_basis(*this) * s);
    }

    inline mat33_t& mat33_t::operator*=(f32 s)
    {
        f32* data = &ix;
       	for (int idx = 0; idx < 9; idx++)
       	{
       		data[idx] *= s;
       	}
       
       	return *this;
    }

    inline mat33_t mat33_t::operator*(const mat33_t& other) const
    {
    	mat33_t copy = *this;
    	copy *= other;
    	return copy;
    }

    inline mat33_t& mat33_t::operator*=(const mat33_t& other)
    {
        mat33_t result;

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

    inline mat33_t init_mat33(const vec3_t& i, const vec3_t& j, const vec3_t& k)
    {
        mat33_t m;
        set_i_basis(m, i);
        set_j_basis(m, j);
        set_k_basis(m, k);
        return m;
    }

    inline mat33_t init_mat33(f32 ix, f32 iy, f32 iz,
                              f32 jx, f32 jy, f32 jz,
                              f32 kx, f32 ky, f32 kz)
    {
        mat33_t m;
        set_i_basis(m, vec3_t(ix, iy, iz));
        set_j_basis(m, vec3_t(jx, jy, jz));
        set_k_basis(m, vec3_t(kx, ky, kz));
        return m;
    }

    inline mat33_t init_mat33(f32* data)
    {
        SM_ASSERT(data);
        mat33_t m;
        memcpy(&m.ix, data, sizeof(f32) * 9);
        return m;
    }

    inline void set_i_basis(mat33_t& m, const vec3_t& i)
    {
        m.ix = i.x;
        m.iy = i.y;
        m.iz = i.z;
    }

    inline void set_j_basis(mat33_t& m, const vec3_t& j)
    {
        m.jx = j.x;
        m.jy = j.y;
        m.jz = j.z;
    }

    inline void set_k_basis(mat33_t& m, const vec3_t& k)
    {
        m.kx = k.x;
        m.ky = k.y;
        m.kz = k.z;
    }

    inline void transpose(mat33_t& m)
    {
        f32* data = &m.ix;
        swap(data[1], data[3]);
        swap(data[2], data[6]);
        swap(data[5], data[7]);
    }

    inline mat33_t transposed(const mat33_t& m)
    {
        mat33_t copy = m;
        transpose(copy);
        return copy;
    }

    inline f32 determinant(const mat33_t& m)
    {
        return m.ix * (m.jy * m.kz - m.jz * m.ky) -
               m.iy * (m.jx * m.kz - m.jz * m.kx) +
               m.iz * (m.jx * m.ky - m.jy * m.kx);
    }
}
