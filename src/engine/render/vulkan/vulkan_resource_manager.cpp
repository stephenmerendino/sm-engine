#include "engine/render/vulkan/vulkan_resource_manager.h"
#include "engine/core/config.h"
#include "engine/core/debug.h"
#include "engine/render/vulkan/vulkan_resources.h"

VkPrimitiveTopology mesh_render_data_get_topology(mesh_id_t mesh_id)
{
    i32 data_index = -1;
    for(i32 i = 0; i < (i32)renderer_globals_get()->loaded_mesh_render_data.size(); i++)
    {
        if(renderer_globals_get()->loaded_mesh_render_data[i]->mesh_id == mesh_id)
        {
            data_index = i;
            break;
        }
    }

    ASSERT(-1 != data_index);

    return renderer_globals_get()->loaded_mesh_render_data[data_index]->topology;
}

pipeline_vertex_input_t  mesh_render_data_get_pipeline_vertex_input(mesh_id_t mesh_id)
{
    i32 data_index = -1;
    for(i32 i = 0; i < (i32)renderer_globals_get()->loaded_mesh_render_data.size(); i++)
    {
        if(renderer_globals_get()->loaded_mesh_render_data[i]->mesh_id == mesh_id)
        {
            data_index = i;
            break;
        }
    }

    ASSERT(-1 != data_index);

    return renderer_globals_get()->loaded_mesh_render_data[data_index]->pipeline_vertex_input;
}

instance_draw_id_t frame_get_or_allocate_instance_draw_data(context_t& context, frame_t& frame)
{
    // TODO: we are duplicating the ds layout code here, clean up
    for(i32 i = 0; i < (i32)frame.mesh_instance_render_data.size(); i++)
    {
        if(!frame.mesh_instance_render_data[i].is_assigned)
        {
            frame.mesh_instance_render_data[i].is_assigned = true;
            std::vector<descriptor_set_layout_t> layout(1, renderer_globals_get()->mesh_instance_render_data_ds_layout);
            std::vector<descriptor_set_t> descriptor_set = descriptor_sets_allocate(context, frame.mesh_instance_render_data_descriptor_pool, layout);
            ASSERT(1 == descriptor_set.size());
            frame.mesh_instance_render_data[i].descriptor_set = descriptor_set[0];
            return (instance_draw_id_t)i;
        }
    }

    mesh_instance_render_data_t new_instance_data;
    new_instance_data.data_buffer = buffer_create(context, BufferType::kUniformBuffer, sizeof(instance_draw_data_t));

    std::vector<descriptor_set_layout_t> layout(1, renderer_globals_get()->mesh_instance_render_data_ds_layout);
    std::vector<descriptor_set_t> descriptor_set = descriptor_sets_allocate(context, frame.mesh_instance_render_data_descriptor_pool, layout);
    ASSERT(1 == descriptor_set.size());
    new_instance_data.descriptor_set = descriptor_set[0];

    new_instance_data.is_assigned = true;

    frame.mesh_instance_render_data.push_back(new_instance_data);
    return (instance_draw_id_t)(frame.mesh_instance_render_data.size() - 1);
}

void frame_update_instance_data(context_t& context, frame_t& frame, instance_draw_id_t instance_id, instance_draw_data_t& instance_draw_data)
{
    buffer_update_data(context, frame.mesh_instance_render_data[instance_id].data_buffer, &instance_draw_data);
}
