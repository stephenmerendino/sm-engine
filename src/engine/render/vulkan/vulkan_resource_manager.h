#pragma once

#include "engine/render/vulkan/vulkan_types.h"
#include "engine/thirdparty/vulkan/vulkan_core.h"

VkPrimitiveTopology mesh_render_data_get_topology(mesh_id_t mesh_id);
pipeline_vertex_input_t  mesh_render_data_get_pipeline_vertex_input(mesh_id_t mesh_id);

instance_draw_id_t frame_get_or_allocate_instance_draw_data(context_t& context, frame_t& frame);
void frame_update_instance_data(context_t& context, frame_t& frame, instance_draw_id_t instance_id, instance_draw_data_t& instance_draw_data);

//mesh_id_t rm_mesh_acquire_or_create();
//void rm_mesh_release(mesh_id_t mesh_id);
//VkPrimitiveTopology rm_mesh_get_topology(mesh_id_t mesh_id);
//VkPrimitiveTopology rm_mesh_get_pipeline_vertex_input(mesh_id_t mesh_id);
//void rm_shaders_create_or_acquire();
//void rm_shaders_release();
//void rm_material_acquire_or_create();
//void rm_material_release();
//void rm_texture_acquire_or_create();
//void rm_texture_release();
//void rm_pipeline_acquire_or_create();
//void rm_pipeline_release();
