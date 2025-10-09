#pragma once

#include "sm/core/types.h"
#include "sm/render/vulkan/vk_mesh_instances.h"

namespace sm
{
    struct arena_t;
	struct window_t;

    typedef void (*collect_mesh_instances_cb_t)(arena_t* frame_allocator, mesh_instances_t* frame_mesh_instances);

	void renderer_init(window_t* window);
    void renderer_begin_frame();
    void renderer_update(f32 ds);
    void renderer_render();

    void renderer_register_collect_mesh_instances_cb(collect_mesh_instances_cb_t cb);
}
