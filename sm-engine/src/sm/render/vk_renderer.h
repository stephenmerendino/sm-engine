#pragma once

#include "sm/core/types.h"

namespace sm
{
	struct window_t;

    typedef u32 mesh_instance_id_t;
    extern const mesh_instance_id_t INVALID_MESH_INSTANCE_ID;

	void renderer_init(window_t* window);
    void renderer_begin_frame();
    void renderer_update_frame(f32 ds);
    void renderer_render_frame();
}
