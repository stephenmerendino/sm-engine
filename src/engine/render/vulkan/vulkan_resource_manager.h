#pragma once

#include "engine/render/vulkan/vulkan_types.h"
#include "engine/thirdparty/vulkan/vulkan_core.h"

void resource_manager_init(context_t& context);
void resource_manager_deinit(context_t& context);

enum PrimitiveMeshType : u32
{
    kCube,
    kAxes
};
static const u32 INVALID_MESH_ID = (u32)-1;

mesh_id_t resource_manager_get_mesh_id(PrimitiveMeshType primitive_type);
mesh_id_t resource_manager_get_mesh_id(const char* filepath);
mesh_id_t resource_manager_load_obj_mesh(context_t& context, const char* obj_filepath);
void resource_manager_mesh_release(context_t& context, mesh_id_t mesh_id);
const mesh_render_data_t* resource_manager_get_mesh_render_data(mesh_id_t mesh_id);
bool resource_manager_is_mesh_loaded(mesh_id_t mesh_id);

//void rm_shaders_create_or_acquire();
//void rm_shaders_release();
//
//void rm_material_acquire_or_create();
//void rm_material_release();
//
//void rm_texture_acquire_or_create();
//void rm_texture_release();
//
//void rm_pipeline_acquire_or_create();
//void rm_pipeline_release();
