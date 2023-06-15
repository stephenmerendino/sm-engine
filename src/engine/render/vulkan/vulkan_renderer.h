#pragma once
#include "engine/render/mesh.h"
#include "engine/render/vulkan/vulkan_types.h"

struct window_t;
struct camera_t;

void renderer_init(window_t* app_window);
renderer_globals_t* renderer_get_globals();
void renderer_set_main_camera(camera_t* camera);
void renderer_begin_frame();
void renderer_update(f32 ds);
void renderer_render_frame();
void renderer_deinit();

