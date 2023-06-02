#include "engine/render/vulkan/vulkan_resource_manager.h"
#include "engine/core/config.h"
#include "engine/core/debug.h"
#include "engine/render/mesh.h"
#include "engine/render/vulkan/vulkan_formats.h"
#include "engine/render/vulkan/vulkan_resources.h"
#include "engine/render/vulkan/vulkan_types.h"
#include "engine/thirdparty/vulkan/vulkan_core.h"

struct managed_meshes_t
{
    std::vector<mesh_id_t> ids;
    std::vector<u32> ref_counts;
    std::vector<mesh_render_data_t*> render_datas;
};
static const u32 INVALID_MESH_INDEX = UINT_MAX;
static managed_meshes_t s_managed_meshes;

struct managed_materials_t
{
    std::vector<const char*> names;
    std::vector<material_id_t> ids;   
    std::vector<u32> ref_counts;
    std::vector<material_t> materials;
};
static const u32 INVALID_MATERIAL_INDEX = UINT_MAX;
static managed_materials_t s_managed_materials;

static
u32 resource_manager_get_mesh_index(mesh_id_t mesh_id)
{
    for(i32 i = 0; i < (i32)s_managed_meshes.ids.size(); i++)
    {
        if(s_managed_meshes.ids[i] == mesh_id)
        {
            return i;
        }
    }

    return INVALID_MESH_INDEX;
}

static
VkPrimitiveTopology primitive_topology_to_vk_topology(PrimitiveTopology topology)
{
    switch(topology)
    {
        case PrimitiveTopology::kTriangleList:  return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        case PrimitiveTopology::kLineList:      return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        case PrimitiveTopology::kPointList:     return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        default: SM_ERROR_MSG("Trying to use an unsupported topology type"); break;
    }

    return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
}

static
void resource_manager_track_mesh(context_t& context, mesh_id_t mesh_id, mesh_t* mesh)
{
    // setup render data
    mesh_render_data_t* mesh_render_data = new mesh_render_data_t;
    mesh_render_data->index_count = (u32)mesh->indices.size();
    mesh_render_data->vertex_count = (u32)mesh->vertices.size();
    mesh_render_data->topology = primitive_topology_to_vk_topology(mesh->topology);

    mesh_render_data->vertex_buffer = buffer_create(context, BufferType::kVertexBuffer, mesh_calc_vertex_buffer_size(mesh));
    buffer_update(context, mesh_render_data->vertex_buffer, context.graphics_command_pool, mesh->vertices.data());

    if (mesh_render_data->index_count > 0)
    {
		mesh_render_data->index_buffer = buffer_create(context, BufferType::kIndexBuffer, mesh_calc_index_buffer_size(mesh));
		buffer_update(context, mesh_render_data->index_buffer, context.graphics_command_pool, mesh->indices.data());
    }

    mesh_render_data->pipeline_vertex_input.input_binding_descs = mesh_get_vertex_input_binding_descs(mesh);
    mesh_render_data->pipeline_vertex_input.input_attr_descs = mesh_get_vertex_input_attr_descs(mesh);
    mesh_render_data->pipeline_vertex_input.vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    mesh_render_data->pipeline_vertex_input.vertex_input_info.vertexBindingDescriptionCount = (u32)mesh_render_data->pipeline_vertex_input.input_binding_descs.size();
    mesh_render_data->pipeline_vertex_input.vertex_input_info.pVertexBindingDescriptions = mesh_render_data->pipeline_vertex_input.input_binding_descs.data();
    mesh_render_data->pipeline_vertex_input.vertex_input_info.vertexAttributeDescriptionCount = (u32)(mesh_render_data->pipeline_vertex_input.input_attr_descs.size());
    mesh_render_data->pipeline_vertex_input.vertex_input_info.pVertexAttributeDescriptions = mesh_render_data->pipeline_vertex_input.input_attr_descs.data();

    s_managed_meshes.ids.push_back(mesh_id);
    s_managed_meshes.ref_counts.push_back(1);  
    s_managed_meshes.render_datas.push_back(mesh_render_data);
}

static
void resource_manager_track_mesh_forever(mesh_id_t mesh_id)
{
    u32 mesh_index = resource_manager_get_mesh_index(mesh_id);
    s_managed_meshes.ref_counts[mesh_index] = UINT_MAX;
}

static
void resource_manager_mesh_render_data_destroy(context_t& context, mesh_render_data_t* mesh_render_data)
{
    buffer_destroy(context, mesh_render_data->index_buffer);
    buffer_destroy(context, mesh_render_data->vertex_buffer);
    delete mesh_render_data;
}

void resource_manager_init(context_t& context)
{
    resource_manager_track_mesh(context, resource_manager_get_mesh_id(PrimitiveMeshType::kAxes), mesh_load_axes());
    resource_manager_track_mesh(context, resource_manager_get_mesh_id(PrimitiveMeshType::kTetrahedron), mesh_load_tetrahedron());
    resource_manager_track_mesh(context, resource_manager_get_mesh_id(PrimitiveMeshType::kHexahedron), mesh_load_cube());
    resource_manager_track_mesh(context, resource_manager_get_mesh_id(PrimitiveMeshType::kOctahedron), mesh_load_octahedron());
    resource_manager_track_mesh(context, resource_manager_get_mesh_id(PrimitiveMeshType::kUvSphere), mesh_load_uv_sphere());
    resource_manager_track_mesh(context, resource_manager_get_mesh_id(PrimitiveMeshType::kPlane), mesh_load_plane());
    resource_manager_track_mesh(context, resource_manager_get_mesh_id(PrimitiveMeshType::kCone), mesh_load_cone());
    resource_manager_track_mesh(context, resource_manager_get_mesh_id(PrimitiveMeshType::kCylinder), mesh_load_cylinder());
    resource_manager_track_mesh(context, resource_manager_get_mesh_id(PrimitiveMeshType::kTorus), mesh_load_torus());

    resource_manager_track_mesh_forever(resource_manager_get_mesh_id(PrimitiveMeshType::kAxes));
    resource_manager_track_mesh_forever(resource_manager_get_mesh_id(PrimitiveMeshType::kTetrahedron));
    resource_manager_track_mesh_forever(resource_manager_get_mesh_id(PrimitiveMeshType::kHexahedron));
    resource_manager_track_mesh_forever(resource_manager_get_mesh_id(PrimitiveMeshType::kOctahedron));
    resource_manager_track_mesh_forever(resource_manager_get_mesh_id(PrimitiveMeshType::kUvSphere));
    resource_manager_track_mesh_forever(resource_manager_get_mesh_id(PrimitiveMeshType::kPlane));
    resource_manager_track_mesh_forever(resource_manager_get_mesh_id(PrimitiveMeshType::kCone));
    resource_manager_track_mesh_forever(resource_manager_get_mesh_id(PrimitiveMeshType::kCylinder));
    resource_manager_track_mesh_forever(resource_manager_get_mesh_id(PrimitiveMeshType::kTorus));
}

void resource_manager_deinit(context_t& context)
{
    for(i32 i = 0; i < (i32)s_managed_meshes.render_datas.size(); i++)
    {
        resource_manager_mesh_render_data_destroy(context, s_managed_meshes.render_datas[i]);
    }
}

mesh_id_t resource_manager_get_mesh_id(PrimitiveMeshType primitive_type)
{
    mesh_id_t mesh_id = INVALID_MESH_ID;

    switch(primitive_type)
    {
        case kAxes: mesh_id = resource_manager_get_mesh_id("primitive-axes"); break;
        case kTetrahedron: mesh_id = resource_manager_get_mesh_id("primitive-tetrahedron"); break;
        case kHexahedron: mesh_id = resource_manager_get_mesh_id("primitive-hexahedron"); break;
        case kOctahedron: mesh_id = resource_manager_get_mesh_id("primitive-octahedron"); break;
        case kUvSphere: mesh_id = resource_manager_get_mesh_id("primitive-uv-sphere"); break;
        case kPlane: mesh_id = resource_manager_get_mesh_id("primitive-plane"); break;
        case kCone: mesh_id = resource_manager_get_mesh_id("primitive-cone"); break;
        case kCylinder: mesh_id = resource_manager_get_mesh_id("primitive-cylinder"); break;
        case kTorus: mesh_id = resource_manager_get_mesh_id("primitive-torus"); break;
    }

    SM_ASSERT_MSG(mesh_id != INVALID_MESH_ID, "A primitive hasn't been set up with resource manager yet.\n");
    return mesh_id;
}

mesh_id_t resource_manager_get_mesh_id(const char* filepath)
{
    return hash(filepath);
}

mesh_id_t resource_manager_load_obj_mesh(context_t& context, const char* obj_filepath)
{
    // get mesh id from filepath
    mesh_id_t mesh_id = resource_manager_get_mesh_id(obj_filepath);
    if(resource_manager_is_mesh_loaded(mesh_id))
    {
        u32 mesh_index = resource_manager_get_mesh_index(mesh_id);
        s_managed_meshes.ref_counts[mesh_index]++;
        return mesh_id;
    }

    mesh_t* obj_mesh = mesh_load_from_obj(obj_filepath);
    resource_manager_track_mesh(context, mesh_id, obj_mesh);
    delete obj_mesh;

    return mesh_id;
}

void resource_manager_mesh_release(context_t& context, mesh_id_t mesh_id)
{
    u32 mesh_index = resource_manager_get_mesh_index(mesh_id);
    SM_ASSERT(INVALID_MESH_INDEX != mesh_index);
    s_managed_meshes.ref_counts[mesh_index]--;

    if(0 == s_managed_meshes.ref_counts[mesh_index])
    {
        resource_manager_mesh_render_data_destroy(context, s_managed_meshes.render_datas[mesh_index]);
        s_managed_meshes.ids.erase(s_managed_meshes.ids.begin() + mesh_index);
        s_managed_meshes.ref_counts.erase(s_managed_meshes.ref_counts.begin() + mesh_index);
        s_managed_meshes.render_datas.erase(s_managed_meshes.render_datas.begin() + mesh_index);
    }
}

const mesh_render_data_t* resource_manager_get_mesh_render_data(mesh_id_t mesh_id)
{
    u32 mesh_index = resource_manager_get_mesh_index(mesh_id);
    SM_ASSERT(INVALID_MESH_INDEX != mesh_index);
    return s_managed_meshes.render_datas[mesh_index];
}

bool resource_manager_is_mesh_loaded(mesh_id_t mesh_id)
{
    return INVALID_MESH_INDEX != resource_manager_get_mesh_index(mesh_id);
}

static
u32 resource_manager_get_material_index(material_id_t mat_id)
{
    for(i32 i = 0; i < (i32)s_managed_materials.ids.size(); i++)
    {
        if(s_managed_materials.ids[i] == mat_id)
        {
            return i;
        }
    }

    return INVALID_MATERIAL_INDEX;
}

static
void material_destroy(context_t& context, material_t& material)
{
    for(i32 i = 0; i < (i32)material.resources.size(); i++)
    {
        texture_destroy(context, material.resources[i].descriptor_resource.texture); 
    }
    descriptor_set_layout_destroy(context, material.descriptor_set_layout);
    pipeline_destroy_shader_stages(context, material.shaders);
}

material_id_t resource_manager_get_material_id(const char* mat_name)
{
    return hash(mat_name); 
}

void resource_manager_material_release(context_t& context, material_id_t mat_id)
{
    u32 mat_index = resource_manager_get_material_index(mat_id);
    SM_ASSERT(INVALID_MATERIAL_INDEX != mat_index);
    s_managed_materials.ref_counts[mat_index]--;

    if(0 == s_managed_materials.ref_counts[mat_index])
    {
        s_managed_materials.names.erase(s_managed_materials.names.begin() + mat_index);
        s_managed_materials.ids.erase(s_managed_materials.ids.begin() + mat_index);
        s_managed_materials.ref_counts.erase(s_managed_materials.ref_counts.begin() + mat_index);
        material_destroy(context, s_managed_materials.materials[mat_index]);
        s_managed_materials.materials.erase(s_managed_materials.materials.begin() + mat_index);
    }
}

const material_t* resource_manager_get_material(material_id_t mat_id)
{
    u32 mat_index = resource_manager_get_material_index(mat_id);
    SM_ASSERT(INVALID_MATERIAL_INDEX != mat_index);
    return &s_managed_materials.materials[mat_index];
}

const material_t* resource_manager_get_material(const char* mat_name)
{
    material_id_t mat_id = resource_manager_get_material_id(mat_name);
    return resource_manager_get_material(mat_id);
}

bool resource_manager_is_material_loaded(material_id_t mat_id)
{
    return INVALID_MATERIAL_INDEX != resource_manager_get_material_index(mat_id);
}

void resource_manager_material_acquire(material_id_t mat_id)
{
    u32 mat_index = resource_manager_get_material_index(mat_id);
    SM_ASSERT(INVALID_MATERIAL_INDEX != mat_index);
    s_managed_materials.ref_counts[mat_index]++;
}

material_id_t resource_manager_track_material_TEMP(const char* mat_name, material_t& mat)
{
    material_id_t mat_id = resource_manager_get_material_id(mat_name);
    if(resource_manager_is_material_loaded(mat_id))
    {
        u32 mat_index = resource_manager_get_material_index(mat_id);
        s_managed_materials.ref_counts[mat_index]++;
        return mat_id;
    }

    s_managed_materials.names.push_back(mat_name);
    s_managed_materials.ids.push_back(mat_id);
    s_managed_materials.ref_counts.push_back(1);  
    s_managed_materials.materials.push_back(mat);

    return mat_id;
}
