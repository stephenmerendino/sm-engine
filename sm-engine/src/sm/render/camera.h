#pragma once

#include "sm/math/vec3.h"
#include "sm/math/mat44.h"

namespace sm
{
	struct camera_t
	{
        vec3_t	world_pos = vec3_t::ZERO;
        f32		world_yaw_degrees = 0.0f;
        f32		world_pitch_degrees = 0.0f;
	};

	void	camera_update(camera_t& camera, f32 ds);
	vec3_t	camera_get_forward(const camera_t& camera);
	vec3_t	camera_get_right(const camera_t& camera);
	vec3_t	camera_get_up(const camera_t& camera);
	mat44_t camera_get_rotation(const camera_t& camera);
	mat44_t camera_get_view_transform(const camera_t& camera);
	void	camera_look_at(camera_t& camera, const vec3_t& look_at_pos, const vec3_t& up_reference = vec3_t::WORLD_UP);
}
