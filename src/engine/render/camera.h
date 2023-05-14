#pragma once

#include "engine/math/mat44.h"

struct camera_t
{
    vec3 m_world_pos = VEC3_ZERO;
    f32 m_world_yaw_deg = 0.0f;
    f32 m_world_pitch_deg = 0.0f;
};

void camera_update(camera_t& camera, f32 ds);
vec3 camera_get_forward(camera_t& camera);
vec3 camera_get_right(camera_t& camera);
vec3 camera_get_up(camera_t& camera);
mat44 camera_get_rotation(camera_t& camera);
void camera_look_at(camera_t& camera, const vec3& look_at_pos_ws, const vec3& up_ref = VEC3_UP);
mat44 camera_get_view_tranform(camera_t& camera);
