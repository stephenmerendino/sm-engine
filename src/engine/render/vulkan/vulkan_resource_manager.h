#pragma once

#include "engine/render/vulkan/vulkan_types.h"

void renderer_globals_create(context_t& context);
void renderer_globals_destroy(context_t& context);
renderer_globals_t* get_renderer_globals();

void mesh_instance_render_data_destroy(context_t& context, mesh_instance_render_data_t& data);
VkPrimitiveTopology mesh_render_data_get_topology(mesh_id_t mesh_id);
pipeline_vertex_input_t  mesh_render_data_get_pipeline_vertex_input(mesh_id_t mesh_id);

instance_draw_id_t frame_get_or_allocate_instance_draw_data(context_t& context, frame_t& frame);
void frame_update_instance_data(context_t& context, frame_t& frame, instance_draw_id_t instance_id, instance_draw_data_t& instance_draw_data);
