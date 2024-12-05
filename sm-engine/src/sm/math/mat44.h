#pragma once

#include "sm/core/types.h"
#include "sm/math/vec3.h"
#include "sm/math/vec4.h"
#include <cstring>

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

namespace sm
{
	struct mat44_t
	{
        f32 ix = 1.0f, iy = 0.0f, iz = 0.0f, iw = 0.0f;
        f32 jx = 0.0f, jy = 1.0f, jz = 0.0f, jw = 0.0f;
        f32 kx = 0.0f, ky = 0.0f, kz = 1.0f, kw = 0.0f;
        f32 tx = 0.0f, ty = 0.0f, tz = 0.0f, tw = 1.0f;

        inline f32* operator[](u32 row);
        inline const f32* operator[](u32 row) const;
        inline mat44_t operator*(f32 s) const;
        inline mat44_t& operator*=(f32 s);
        inline mat44_t operator*(const mat44_t& other) const;
        inline mat44_t& operator*=(const mat44_t& other);
	};

    inline mat44_t  init_mat44(const vec4_t& i, const vec4_t& j, const vec4_t& k, const vec4_t& t);
    inline mat44_t  init_mat44(f32 ix, f32 iy, f32 iz, f32 iw,
                               f32 jx, f32 jy, f32 jz, f32 jw,
                               f32 kx, f32 ky, f32 kz, f32 kw,
                               f32 tx, f32 ty, f32 tz, f32 tw);
    inline mat44_t  init_mat44(f32* data);
    inline mat44_t  init_mat44(const vec3_t& i, const vec3_t& j, const vec3_t& k);
    inline mat44_t  init_mat44(const vec3_t& i, const vec3_t& j, const vec3_t& k, const vec3_t& t);

	inline mat44_t  init_scale(f32 uniform_scale);
	inline mat44_t  init_scale(f32 i, f32 j, f32 k);
	inline mat44_t  init_scale(const vec3_t& t);

	inline mat44_t  init_rotation_x_rads(f32 x_rads);
	inline mat44_t  init_rotation_y_rads(f32 y_rads);
	inline mat44_t  init_rotation_z_rads(f32 z_rads);
	inline mat44_t  init_rotation_x_degs(f32 x_degs);
	inline mat44_t  init_rotation_y_degs(f32 y_degs);
	inline mat44_t  init_rotation_z_degs(f32 z_degs);
	inline mat44_t  init_rotation_around_axis_rads(const vec3_t& axis, f32 rads);
	inline mat44_t  init_rotation_around_axis_degs(const vec3_t& axis, f32 degs);

	inline mat44_t  init_translation(f32 tx, f32 ty, f32 tz);
	inline mat44_t  init_translation(const vec3_t& t);

	inline mat44_t  init_basis_from_i(const vec3_t& inI);
	inline mat44_t  init_basis_from_j(const vec3_t& inJ);
	inline mat44_t  init_basis_from_k(const vec3_t& inK);
	inline mat44_t  init_perspective_proj(f32 vertical_fov_deg, f32 n, f32 f, f32 aspect);

    inline vec4_t   get_i_basis(const mat44_t& m) { return vec4_t(m.ix, m.iy, m.iz, m.iw); }
    inline vec4_t   get_j_basis(const mat44_t& m) { return vec4_t(m.jx, m.jy, m.jz, m.jw); }
    inline vec4_t   get_k_basis(const mat44_t& m) { return vec4_t(m.kx, m.ky, m.kz, m.kw); }
    inline vec4_t   get_t_basis(const mat44_t& m) { return vec4_t(m.tx, m.ty, m.tz, m.tw); }
    inline void     set_i_basis(mat44_t& m, const vec4_t& i);
    inline void     set_j_basis(mat44_t& m, const vec4_t& j);
    inline void     set_k_basis(mat44_t& m, const vec4_t& k);
    inline void     set_t_basis(mat44_t& m, const vec4_t& t);

    inline void     transpose(mat44_t& m);
    inline mat44_t  transposed(const mat44_t& m);
    inline f32      determinant(const mat44_t& m);
    inline void     inverse(mat44_t& m);
    inline mat44_t  inversed(const mat44_t& m);
    inline void     fast_ortho_inverse(mat44_t& m);
    inline mat44_t  fast_ortho_inversed(const mat44_t& m);

	inline void     scale(mat44_t& m, f32 uniform_scale);
	inline void     scale(mat44_t& m, f32 i, f32 j, f32 k);
	inline void     scale(mat44_t& m, const vec3_t& ijk);
	inline mat44_t  scaled(const mat44_t& m, f32 uniform_scale);
	inline mat44_t  scaled(const mat44_t& m, f32 i, f32 j, f32 k);
	inline mat44_t  scaled(const mat44_t& m, const vec3_t& ijk);
	inline void     set_scale(mat44_t& m, f32 uniform_scale);
	inline void     set_scale(mat44_t& m, f32 i, f32 j, f32 k);
	inline void     set_scale(mat44_t& m, const vec3_t& ijk);

	inline void     translate(mat44_t& m, f32 tx, f32 ty, f32 tz);
	inline void     translate(mat44_t& m, const vec3_t& t);
	inline mat44_t  translated(const mat44_t& m, f32 tx, f32 ty, f32 tz);
	inline mat44_t  translated(const mat44_t& m, const vec3_t& t);
	inline void     set_translation(mat44_t& m, f32 tx, f32 ty, f32 tz);
	inline void     set_translation(mat44_t& m, const vec3_t& t);

	inline void     rotate_x_rads(mat44_t& m, f32 x_rads);
	inline void     rotate_y_rads(mat44_t& m, f32 y_rads);
	inline void     rotate_z_rads(mat44_t& m, f32 z_rads);
	inline void     rotate_x_degs(mat44_t& m, f32 x_degs);
	inline void     rotate_y_degs(mat44_t& m, f32 y_degs);
	inline void     rotate_z_degs(mat44_t& m, f32 z_degs);
	inline void     rotate_around_axis_rads(mat44_t& m, const vec3_t& axis, f32 rads);
	inline void     rotate_around_axis_degs(mat44_t& m, const vec3_t& axis, f32 degs);
    inline mat44_t  get_rotation(const mat44_t& m);
	inline void     set_rotation(mat44_t& m, const mat44_t& rotation);

	inline vec3_t   transform_dir(const vec3_t& dir);
	inline vec3_t   transform_point(const vec3_t& p);

    inline mat44_t init_mat44(const vec4_t& i, const vec4_t& j, const vec4_t& k, const vec4_t& t)
    {
        mat44_t m;
        set_i_basis(m, i);
        set_j_basis(m, j);
        set_k_basis(m, k);
        set_t_basis(m, t);
        return m;
    }

    inline mat44_t init_mat44(f32 ix, f32 iy, f32 iz, f32 iw,
                              f32 jx, f32 jy, f32 jz, f32 jw,
                              f32 kx, f32 ky, f32 kz, f32 kw,
                              f32 tx, f32 ty, f32 tz, f32 tw)
    {
        mat44_t m;
        set_i_basis(m, vec4_t(ix, iy, iz, iw));
        set_j_basis(m, vec4_t(jx, jy, jz, jw));
        set_k_basis(m, vec4_t(kx, ky, kz, kw));
        set_t_basis(m, vec4_t(tx, ty, tz, tw));
        return m;
    }

    inline mat44_t init_mat44(f32* data)
    {
        SM_ASSERT(data);
        mat44_t m;
        memcpy(&m.ix, data, sizeof(f32) * 16);
    }

    inline mat44_t init_mat44(const vec3_t& i, const vec3_t& j, const vec3_t& k)
    {
        return init_mat44(i.x, i.y, i.z, 0.0f, 
                          j.x, j.y, j.z, 0.0f, 
                          k.x, k.y, k.z, 0.0f, 
                          0.0f, 0.0f, 0.0f, 1.0f);
    }

    inline mat44_t init_mat44(const vec3_t& i, const vec3_t& j, const vec3_t& k, const vec3_t& t)
    {
        return init_mat44(i.x, i.y, i.z, 0.0f, 
                          j.x, j.y, j.z, 0.0f, 
                          k.x, k.y, k.z, 0.0f, 
                          t.x, t.y, t.z, 1.0f);
    }

    inline mat44_t init_scale(f32 uniform_scale)
    {
        mat44_t m;
        set_scale(m, uniform_scale);
        return m;
    }

    inline mat44_t init_scale(f32 i, f32 j, f32 k)
    {
        mat44_t m;
        set_scale(m, i, j, k);
        return m;
    }

    inline mat44_t init_scale(const vec3_t& t)
    {
        mat44_t m;
        set_scale(m, t);
        return m;
    }

    inline mat44_t init_rotation_x_rads(f32 x_rads)
    {
        mat44_t m;
        rotate_x_rads(m, x_rads);
        return m;
    }

    inline mat44_t init_rotation_y_rads(f32 y_rads)
    {
        mat44_t m;
        rotate_y_rads(m, y_rads);
        return m;
    }

    inline mat44_t init_rotation_z_rads(f32 z_rads)
    {
        mat44_t m;
        rotate_z_rads(m, z_rads);
        return m;
    }

    inline mat44_t init_rotation_x_degs(f32 x_degs)
    {
        mat44_t m;
        rotate_x_degs(m, x_degs);
        return m;
    }

    inline mat44_t init_rotation_y_degs(f32 y_degs)
    {
        mat44_t m;
        rotate_y_degs(m, y_degs);
        return m;
    }

    inline mat44_t init_rotation_z_degs(f32 z_degs)
    {
        mat44_t m;
        rotate_z_degs(m, z_degs);
        return m;
    }

    inline mat44_t init_rotation_around_axis_rads(const vec3_t& axis, f32 rads)
    {
        mat44_t m;
        rotate_around_axis_rads(m, axis, rads);
        return m;
    }

    inline mat44_t init_rotation_around_axis_degs(const vec3_t& axis, f32 degs)
    {
        mat44_t m;
        rotate_around_axis_degs(m, axis, degs);
        return m;
    }

    inline mat44_t init_translation(f32 tx, f32 ty, f32 tz)
    {
        mat44_t m;
        translate(m, tx, ty, tz);
        return m;
    }

    inline mat44_t init_translation(const vec3_t& t)
    {
        mat44_t m;
        translate(m, t);
        return m;
    }

    inline mat44_t init_basis_from_i(const vec3_t& i)
    {
        vec3_t i_norm = normalized(i);

        // account for i_norm pointing directly up or down when trying to build basis
        vec3_t j_norm = vec3_t::ZERO;
        f32 cos_angle_abs = abs(dot(i_norm, vec3_t::WORLD_UP));
        if (almost_equals(cos_angle_abs, 1.0f))
        {
            j_norm = normalized(cross(i_norm, vec3_t::WORLD_FORWARD));
        }
        else
        {
            j_norm = normalized(cross(vec3_t::WORLD_UP, i_norm));
        }

        vec3_t k_norm = cross(i_norm, j_norm);

        return init_mat44(i_norm, j_norm, k_norm);
    }

    inline mat44_t init_basis_from_j(const vec3_t& j)
    {
        vec3_t j_norm = normalized(j);

        // account for j_norm pointing directly up or down when trying to build basis
        vec3_t i_norm = vec3_t::ZERO;
        f32 cos_angle_abs = abs(dot(j_norm, vec3_t::WORLD_UP));
        if (almost_equals(cos_angle_abs, 1.0f))
        {
            i_norm = normalized(cross(vec3_t::WORLD_LEFT, j_norm));
        }
        else
        {
            i_norm = normalized(cross(j_norm, vec3_t::WORLD_UP));
        }

        vec3_t k_norm = cross(i_norm, j_norm);

        return init_mat44(i_norm, j_norm, k_norm);
    }

    inline mat44_t init_basis_from_k(const vec3_t& k)
    {
        vec3_t k_norm = normalized(k);

        // account for k_norm pointing directly up or down when trying to build basis
        vec3_t i_norm = vec3_t::ZERO;
        f32 cos_angle_abs = abs(dot(k_norm, vec3_t::WORLD_UP));
        if (almost_equals(cos_angle_abs, 1.0f))
        {
            i_norm = normalized(cross(vec3_t::WORLD_FORWARD, k_norm));
        }
        else
        {
            i_norm = normalized(cross(k_norm, vec3_t::WORLD_UP));
        }

        vec3_t j_norm = cross(k_norm, i_norm);

        return init_mat44(i_norm, j_norm, k_norm);
    }

	inline mat44_t init_perspective_proj(f32 vertical_fov_deg, f32 n, f32 f, f32 aspect)
    {
        f32 focal_len = 1.0f / tan_deg(vertical_fov_deg * 0.5f);
        return init_mat44(focal_len / aspect, 0.0f, 0.0f, 0.0f,
                          0.0f, -focal_len, 0.0f, 0.0f,
                          0.0f, 0.0f, f / (f - n), 1.0f,
                          0.0f, 0.0f, -n * f / (f - n), 0.0f);
    }

    inline f32* mat44_t::operator[](u32 row)
    {
        SM_ASSERT(row >= 0 && row <= 3);
        f32* row_start = &ix + (row * 4);
        return row_start;
    }

    inline const f32* mat44_t::operator[](u32 row) const
    {
        SM_ASSERT(row >= 0 && row <= 3);
        const f32* row_start = &ix + (row * 4);
        return row_start;
    }

    inline mat44_t mat44_t::operator*(f32 s) const
    {
        return init_mat44(get_i_basis(*this) * s, 
                          get_j_basis(*this) * s,
                          get_k_basis(*this) * s,
                          get_t_basis(*this) * s);
    }

    inline mat44_t& mat44_t::operator*=(f32 s)
    {
        f32* data = &ix;
       	for (int idx = 0; idx < 16; idx++)
       	{
       		data[idx] *= s;
       	}
       
       	return *this;
    }

    inline mat44_t mat44_t::operator*(const mat44_t& other) const
    {
    	mat44_t copy = *this;
    	copy *= other;
    	return copy;
    }

    inline mat44_t& mat44_t::operator*=(const mat44_t& other)
    {
    	mat44_t result;
    
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

    inline void set_i_basis(mat44_t& m, const vec4_t& i)
    {
        m.ix = i.x;
        m.iy = i.y;
        m.iz = i.z;
        m.iw = i.w;
    }

    inline void set_j_basis(mat44_t& m, const vec4_t& j)
    {
        m.jx = j.x;
        m.jy = j.y;
        m.jz = j.z;
        m.jw = j.w;
    }

    inline void set_k_basis(mat44_t& m, const vec4_t& k)
    {
        m.kx = k.x;
        m.ky = k.y;
        m.kz = k.z;
        m.kw = k.w;
    }

    inline void set_t_basis(mat44_t& m, const vec4_t& t)
    {
        m.tx = t.x;
        m.ty = t.y;
        m.tz = t.z;
        m.tw = t.w;
    }

    //inline void     transpose(mat44_t& m);
    //inline mat44_t  transposed(const mat44_t& m);
    //inline f32      determinant(const mat44_t& m);
    //inline void     inverse(mat44_t& m);
    //inline mat44_t  inversed(const mat44_t& m);
    //inline void     fast_ortho_inverse(mat44_t& m);
    //inline mat44_t  fast_ortho_inversed(const mat44_t& m);

	//inline void     scale(mat44_t& m, f32 uniform_scale);
	//inline void     scale(mat44_t& m, f32 i, f32 j, f32 k);
	//inline void     scale(mat44_t& m, const vec3_t& ijk);
	//inline mat44_t  scaled(const mat44_t& m, f32 uniform_scale);
	//inline mat44_t  scaled(const mat44_t& m, f32 i, f32 j, f32 k);
	//inline mat44_t  scaled(const mat44_t& m, const vec3_t& ijk);
	//inline void     set_scale(mat44_t& m, f32 uniform_scale);
	//inline void     set_scale(mat44_t& m, f32 i, f32 j, f32 k);
	//inline void     set_scale(mat44_t& m, const vec3_t& ijk);

	//inline void     translate(mat44_t& m, f32 tx, f32 ty, f32 tz);
	//inline void     translate(mat44_t& m, const vec3_t& t);
	//inline mat44_t  translated(const mat44_t& m, f32 tx, f32 ty, f32 tz);
	//inline mat44_t  translated(const mat44_t& m, const vec3_t& t);
	//inline void     set_translation(mat44_t& m, f32 tx, f32 ty, f32 tz);
	//inline void     set_translation(mat44_t& m, const vec3_t& t);

	//inline void     rotate_x_rads(mat44_t& m, f32 x_rads);
	//inline void     rotate_y_rads(mat44_t& m, f32 y_rads);
	//inline void     rotate_z_rads(mat44_t& m, f32 z_rads);
	//inline void     rotate_x_degs(mat44_t& m, f32 x_degs);
	//inline void     rotate_y_degs(mat44_t& m, f32 y_degs);
	//inline void     rotate_z_degs(mat44_t& m, f32 z_degs);
	//inline void     rotate_around_axis_rads(mat44_t& m, const vec3_t& axis, f32 rads);
	//inline void     rotate_around_axis_degs(mat44_t& m, const vec3_t& axis, f32 degs);
    //inline mat44_t  get_rotation(const mat44_t& m);
	//inline void     set_rotation(mat44_t& m, const mat44_t& rotation);

	//inline vec3_t   transform_dir(const vec3_t& dir);
	//inline vec3_t   transform_point(const vec3_t& p);
}
