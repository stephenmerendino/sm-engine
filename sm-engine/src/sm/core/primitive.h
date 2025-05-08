#pragma once

#include "sm/math/vec3.h"

namespace sm
{
	struct ray_t
	{
		vec3_t origin;
		vec3_t dir;
		f32 length;
	};

	struct sphere_t
	{
		vec3_t center;
		f32 radius;
	};

	struct cylinder_t
	{
		vec3_t center;
		vec3_t up_dir;
		f32 height;
		f32 radius;
	};

	struct cube_t
	{
		vec3_t center;
		f32 half_width;
	};

	struct aabb_t
	{
		vec3_t min;
		vec3_t max;
	};

	struct torus_t
	{
		vec3_t center;
		vec3_t normal;
		f32 radius;
		f32 ring_thickness;
	};

	struct plane_t
	{
		vec3_t normal;
		f32 distance;
	};

	struct quad_t
	{
		vec3_t center;
		vec3_t normal;
		f32 half_width;
		f32 half_height;
	};
}
