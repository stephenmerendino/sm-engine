#pragma once

struct window_t;
struct camera_t;

void renderer_init(window_t* app_window);
void renderer_set_main_camera(camera_t* camera);
void renderer_render_frame();
void renderer_deinit();
