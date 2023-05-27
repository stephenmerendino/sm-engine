#pragma once

#include "engine/render/mesh.h"

struct window_t;
struct camera_t;

void renderer_init(window_t* app_window);
void renderer_set_main_camera(camera_t* camera);
void renderer_update(f32 ds);
void renderer_render_frame();
void renderer_deinit();

void renderer_load_mesh(mesh_t* mesh);
void renderer_unload_mesh(mesh_id_t mesh_id);
