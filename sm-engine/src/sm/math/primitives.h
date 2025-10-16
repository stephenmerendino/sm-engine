#pragma once

#include "sm/math/vec3.h"

namespace sm
{
	struct ray_t
	{
		vec3_t origin = vec3_t::ZERO;
		vec3_t dir = vec3_t::ZERO;
		f32 length = 0.0f;
	};

	struct ray_hit_t
	{
		bool did_hit = false;
		vec3_t hit_pos[2] = { vec3_t::ZERO, vec3_t::ZERO };
	};

	struct sphere_t
	{
		vec3_t center = vec3_t::ZERO;
		f32 radius = 0.0f;
	};

	struct cube_t
	{
		vec3_t center;
		f32 half_width;
	};

	//struct cylinder_t
	//{
	//	vec3_t center;
	//	vec3_t up_dir;
	//	f32 height;
	//	f32 radius;
	//};

	//struct aabb_t
	//{
	//	vec3_t min;
	//	vec3_t max;
	//};

	//struct torus_t
	//{
	//	vec3_t center;
	//	vec3_t normal;
	//	f32 radius;
	//	f32 ring_thickness;
	//};

	//struct plane_t
	//{
	//	vec3_t normal;
	//	f32 distance;
	//};

	//struct quad_t
	//{
	//	vec3_t center;
	//	vec3_t normal;
	//	f32 half_width;
	//	f32 half_height;
	//};

	bool ray_intersect_sphere(ray_hit_t& out_ray_hit, const ray_t& ray, const sphere_t& cylinder);
	//bool ray_intersect_cylinder(ray_hit_t& out_ray_hit, const ray_t& ray, const cylinder_t& cylinder);
}
