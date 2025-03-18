#include "sm/render/camera.h"
#include "sm/io/input.h"

using namespace sm;

void sm::camera_update(camera_t& camera, f32 ds)
{
	vec3_t forward = camera_get_forward(camera);
	vec3_t right = camera_get_right(camera);
	vec3_t up = camera_get_up(camera);

	f32 move_speed = 3.0f; // meters per second
	if (input_is_key_down(key_code_t::KEY_SHIFT))
	{
		move_speed *= 3.0f;
	}

	vec3_t movement = vec3_t::ZERO;

	// position
	if (input_is_key_down(key_code_t::KEY_W))
	{
		movement += forward * move_speed * ds;
	}

	if (input_is_key_down(key_code_t::KEY_S))
	{
		movement += -forward * move_speed * ds;
	}

	if (input_is_key_down(key_code_t::KEY_D))
	{
		movement += right * move_speed * ds;
	}

	if (input_is_key_down(key_code_t::KEY_A))
	{
		movement += -right * move_speed * ds;
	}

	if (input_is_key_down(key_code_t::KEY_E))
	{
		movement += up * move_speed * ds;
	}

	if (input_is_key_down(key_code_t::KEY_Q))
	{
		movement += -up * move_speed * ds;
	}

	camera.world_pos += movement;

	// rotation
	const f32 rotate_speed = 15000.0f;
	vec2_t mouse_movement = input_get_mouse_movement_normalized();
	if (mouse_movement.x != 0.0f)
	{
		f32 rotation_deg = -mouse_movement.x * rotate_speed * ds;
		camera.world_yaw_degrees += rotation_deg;
	}

	if (mouse_movement.y != 0.0f)
	{
		f32 rotation_deg = mouse_movement.y * rotate_speed * ds;
		camera.world_pitch_degrees += rotation_deg;
		camera.world_pitch_degrees = clamp(camera.world_pitch_degrees, -89.9f, 89.9f);
	}
}

vec3_t sm::camera_get_forward(const camera_t& camera)
{
	mat44_t rotation = camera_get_rotation(camera);
	return to_vec3(get_i_basis(rotation));
}

vec3_t sm::camera_get_right(const camera_t& camera)
{
	mat44_t rotation = camera_get_rotation(camera);
	return to_vec3(-get_j_basis(rotation));
}

vec3_t sm::camera_get_up(const camera_t& camera)
{
	mat44_t rotation = camera_get_rotation(camera);
	return to_vec3(get_k_basis(rotation));
}

mat44_t sm::camera_get_rotation(const camera_t& camera)
{
	mat44_t pitch = init_rotation_y_degs(camera.world_pitch_degrees);
	mat44_t yaw = init_rotation_z_degs(camera.world_yaw_degrees);

	return pitch * yaw;
}

mat44_t sm::camera_get_view_transform(const camera_t& camera)
{
	// Transform from world space to camera space by swapping x=>z, y=>-x, z=>y
	// World is right handed z up, Camera is left handed Y up / z forward
	mat44_t change_of_basis = mat44_t(0.0f, 0.0f, 1.0f, 0.0f,
									 -1.0f, 0.0f, 0.0f, 0.0f,
									  0.0f, 1.0f, 0.0f, 0.0f,
									  0.0f, 0.0f, 0.0f, 1.0f);

	mat44_t camera_to_world = camera_get_rotation(camera);
	mat44_t world_to_camera = transposed(camera_to_world);

	vec3_t world_translation = camera.world_pos;
	vec3_t world_to_camera_translation = -1.0f * transform_point(world_to_camera, world_translation);

	mat44_t view_transform = world_to_camera * change_of_basis;
	vec3_t view_translation = transform_point(change_of_basis, world_to_camera_translation);
	set_translation(view_transform, world_to_camera_translation);

	return view_transform;
}

void sm::camera_look_at(camera_t& camera, const vec3_t& look_at_pos, const vec3_t& up_reference)
{
	vec3_t view_dir = look_at_pos - camera.world_pos;
	normalize(view_dir);

	vec3_t view_up = up_reference - (dot(up_reference, view_dir) * view_dir);
	normalize(view_up);

	vec3_t view_dir_xy = vec3_t(view_dir.x, view_dir.y, 0.0f);
	normalize(view_dir_xy);

	f32 cos_yaw_rads = dot(view_dir_xy, vec3_t::WORLD_FORWARD);
	f32 cos_pitch_rads = dot(view_dir, up_reference);

	//#TODO(smerendino): Is there a more elegant way to get the correctly signed angle
	f32 angle_sign = dot(view_dir_xy, vec3_t::WORLD_LEFT) > 0.0f ? 1.0f : -1.0f;
	f32 yaw_rads = acosf(cos_yaw_rads) * angle_sign;
	f32 pitch_rads = acosf(cos_pitch_rads);

	camera.world_yaw_degrees = rad_to_deg(yaw_rads);

	f32 pitch_deg = rad_to_deg(pitch_rads);
	camera.world_pitch_degrees = remap(pitch_deg, 0.0f, 180.0f, -90.0f, 90.0f); // remap angle between view dir and up to relative to xy plane
}
