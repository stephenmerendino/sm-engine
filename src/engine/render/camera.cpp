#include "engine/render/Camera.h"
#include "engine/input/input.h"
#include "engine/math/mat33.h"
#include "engine/math/vec3.h"

void camera_update(camera_t& camera, f32 ds)
{
	vec3 forward = camera_get_forward(camera);
	vec3 right = camera_get_right(camera);
	vec3 up = camera_get_up(camera);

	f32 move_speed = 3.0f; // meters per second
    if(input_is_key_down(KeyCode::KEY_SHIFT))
    {
        move_speed *= 3.0f;    
    }

    vec3 movement = VEC3_ZERO;

	// Position
	if (input_is_key_down(KeyCode::KEY_W))
	{
		movement += forward * move_speed * ds;
	}

	if (input_is_key_down(KeyCode::KEY_S))
	{
		movement += -forward * move_speed * ds;
	}

	if (input_is_key_down(KeyCode::KEY_D))
	{
		movement += right * move_speed * ds;
	}

	if (input_is_key_down(KeyCode::KEY_A))
	{
		movement += -right * move_speed * ds;
	}

	if (input_is_key_down(KeyCode::KEY_E))
	{ 
		movement += up * move_speed * ds;
	}

	if (input_is_key_down(KeyCode::KEY_Q))
	{
		movement += -up * move_speed * ds;
	}

    camera.m_world_pos += movement;

	// Rotation
	const f32 rotate_speed = 5000.0f;
	vec2 mouse_movement = input_get_mouse_movement();
	if (mouse_movement.x != 0.0f)
	{
		f32 rotation_deg = -mouse_movement.x * rotate_speed * ds;		
		camera.m_world_yaw_deg += rotation_deg;
	}

	if (mouse_movement.y != 0.0f)
	{
		f32 rotation_deg = mouse_movement.y * rotate_speed * ds;
		camera.m_world_pitch_deg += rotation_deg;
		camera.m_world_pitch_deg = clamp(camera.m_world_pitch_deg, -89.9f, 89.9f);
	}
}

vec3 camera_get_forward(camera_t& camera)
{
    return camera_get_rotation(camera).i.xyz;
}

vec3 camera_get_right(camera_t& camera)
{
    return -camera_get_rotation(camera).j.xyz;
}

vec3 camera_get_up(camera_t& camera)
{
    return camera_get_rotation(camera).k.xyz;
}

mat44 camera_get_rotation(camera_t& camera)
{
	mat44 pitch = make_rotation_y_deg(camera.m_world_pitch_deg);
	mat44 yaw = make_rotation_z_deg(camera.m_world_yaw_deg);

	return pitch * yaw;
}

void camera_look_at(camera_t& camera, const vec3& look_at_pos_ws, const vec3& up_ref)
{
	vec3 view_dir = look_at_pos_ws - camera.m_world_pos;
    normalize(view_dir);

	vec3 view_up = up_ref - (dot(up_ref, view_dir) * view_dir);
    normalize(view_up);

	vec3 view_dir_xy = make_vec3(view_dir.x, view_dir.y, 0.0f);
    normalize(view_dir_xy);

	f32 cos_yaw_rads = dot(view_dir_xy, VEC3_FORWARD);
	f32 cos_pitch_rads = dot(view_dir, up_ref);

	//#TODO(smerendino): Is there a more elegant way to get the correctly signed angle
	f32 angle_sign = dot(view_dir_xy, VEC3_LEFT) > 0.0f ? 1.0f : -1.0f;
	f32 yaw_rads = acosf(cos_yaw_rads) * angle_sign;
	f32 pitch_rads = acosf(cos_pitch_rads);

	camera.m_world_yaw_deg = rad_to_deg(yaw_rads);
	
	f32 pitch_deg = rad_to_deg(pitch_rads);
	camera.m_world_pitch_deg = remap(pitch_deg, 0.0f, 180.0f, -90.0f, 90.0f); // remap angle between view dir and up to relative to xy plane
}

mat44 camera_get_view_tranform(camera_t& camera)
{
	// Transform from world space to camera space by swapping x=>z, y=>-x, z=>y
	// World is right handed z up, Camera is left handed Y up / z forward
	mat44 change_of_basis = make_mat44(0.0f, 0.0f, 1.0f, 0.0f,
		                               -1.0f, 0.0f, 0.0f, 0.0f,
		                               0.0f, 1.0f, 0.0f, 0.0f,
	                                   0.0f, 0.0f, 0.0f, 1.0f);

	mat44 camera_to_world = camera_get_rotation(camera);
	mat44 world_to_camera = get_transposed(camera_to_world);

	vec3 world_translation = camera.m_world_pos;
	vec3 world_to_camera_translation = -1.0f * transform_point(world_to_camera, world_translation);
	
	mat44 view_transform = world_to_camera * change_of_basis;
	vec3 view_translation = transform_point(change_of_basis, world_to_camera_translation);
    view_transform.t.xyz = view_translation;

	return view_transform;
}
