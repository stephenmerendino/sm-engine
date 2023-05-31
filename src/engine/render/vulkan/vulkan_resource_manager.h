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

material_id_t resource_manager_track_material_TEMP(const char* mat_name, material_t& mat);
void resource_manager_material_acquire(material_id_t mat_id);
void resource_manager_material_release(context_t& context, material_id_t mat_id);
material_id_t resource_manager_get_material_id(const char* mat_name);
const material_t* resource_manager_get_material(material_id_t mat_id);
const material_t* resource_manager_get_material(const char* mat_name);
bool resource_manager_is_material_loaded(material_id_t mat_id);
