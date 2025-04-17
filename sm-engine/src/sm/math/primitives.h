#pragma once

#include "sm/math/vec3.h"

namespace sm
{
	struct ray_t
	{
		vec3_t origin;
		vec3_t dir;
	};

	struct cylinder_t
	{
		vec3_t bottom_center;
		vec3_t direction;
		f32 height;
		f32 radius;
	};

	struct ray_hit_t
	{
		vec3_t hit_pos;
	};

	bool ray_intersect_cylinder(ray_hit_t& out_ray_hit, const ray_t& ray, const cylinder_t& cylinder);
}
