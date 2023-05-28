#include "engine/render/vulkan/vulkan_resource_manager.h"
#include "engine/core/debug.h"
#include "engine/render/vulkan/vulkan_resources.h"

static renderer_globals_t* s_globals = nullptr;

void renderer_globals_create(context_t& context)
{
    s_globals = new renderer_globals_t;
    s_globals->debug_render = false;

    // render pass
    {
        render_pass_attachments_t attachments;
        render_pass_add_attachment(attachments, context.swapchain.format, context.device.max_num_msaa_samples, 
                                                 VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 
                                                 VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, 
                                                 VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, 
                                                 0);
        render_pass_add_attachment(attachments, context.device.depth_format, context.device.max_num_msaa_samples, 
                                                 VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 
                                                 VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE, 
                                                 VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, 
                                                 0);
        render_pass_add_attachment(attachments, context.swapchain.format, VK_SAMPLE_COUNT_1_BIT, 
                                                 VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
                                                 VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE, 
                                                 VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, 
                                                 0);

        subpasses_t subpasses;
        subpasses_add_attachment_ref(subpasses, 0, SubpassAttachmentRefType::COLOR, 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        subpasses_add_attachment_ref(subpasses, 0, SubpassAttachmentRefType::DEPTH, 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
        subpasses_add_attachment_ref(subpasses, 0, SubpassAttachmentRefType::RESOLVE, 2, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

        subpass_dependencies_t subpass_dependencies;
        subpass_add_dependency(subpass_dependencies, VK_SUBPASS_EXTERNAL, 
                                                     0, 
                                                     VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                                                     VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                                                     0,
                                                     VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);

        s_globals->main_draw_render_pass = render_pass_create(context, attachments, subpasses, subpass_dependencies);
    }

    // global data descriptor pool and set
    {
        descriptor_set_layout_bindings_t bindings;
        descriptor_set_layout_add_binding(bindings, 0, 1, VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_ALL);
        s_globals->global_data_ds_layout = descriptor_set_layout_create(context, bindings);

        const i32 EMPTY_SET = 1;
        const i32 MAX_SETS = 1;
        descriptor_pool_sizes_t pool_sizes;
        descriptor_pool_add_size(pool_sizes, VK_DESCRIPTOR_TYPE_SAMPLER, MAX_SETS);
        s_globals->global_data_dp = descriptor_pool_create(context, pool_sizes, MAX_SETS + EMPTY_SET);

        std::vector<descriptor_set_layout_t> layouts(1, s_globals->global_data_ds_layout);
        std::vector<descriptor_set_t> sets = descriptor_sets_allocate(context, s_globals->global_data_dp, layouts);
        s_globals->global_ds = sets[0];
    }

    // frame data descriptor pool and set
    {
        descriptor_set_layout_bindings_t bindings;
        descriptor_set_layout_add_binding(bindings, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL);
        s_globals->frame_data_ds_layout = descriptor_set_layout_create(context, bindings);

        const i32 INITIAL_MAX_SETS = 1;
        descriptor_pool_sizes_t pool_sizes;
        descriptor_pool_add_size(pool_sizes, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, INITIAL_MAX_SETS);
        s_globals->frame_data_dp = descriptor_pool_create(context, pool_sizes, INITIAL_MAX_SETS);
        
        std::vector<descriptor_set_layout_t> layouts(1, s_globals->frame_data_ds_layout);
        std::vector<descriptor_set_t> sets = descriptor_sets_allocate(context, s_globals->frame_data_dp, layouts);
        s_globals->frame_ds = sets[0];
    }

    // material data descriptor pool and set
    {
        descriptor_set_layout_bindings_t bindings;
        descriptor_set_layout_add_binding(bindings, 0, 1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT);
        s_globals->material_data_ds_layout = descriptor_set_layout_create(context, bindings);

        const i32 INITIAL_MAX_SETS = 100;
        descriptor_pool_sizes_t pool_sizes;
        descriptor_pool_add_size(pool_sizes, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, INITIAL_MAX_SETS);
        s_globals->material_data_dp = descriptor_pool_create(context, pool_sizes, INITIAL_MAX_SETS);

        std::vector<descriptor_set_layout_t> layouts(1, s_globals->material_data_ds_layout);
        std::vector<descriptor_set_t> sets = descriptor_sets_allocate(context, s_globals->material_data_dp, layouts);
        s_globals->material_ds = sets[0];
    }

    // mesh instance data descriptor set layout
    {
        descriptor_set_layout_bindings_t bindings;
        descriptor_set_layout_add_binding(bindings, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
        s_globals->mesh_instance_render_data_ds_layout = descriptor_set_layout_create(context, bindings);
    }


    // empty descriptor set for binding in places where we don't need a ds for materials
    {
        descriptor_set_layout_bindings_t bindings;
        descriptor_set_layout_t empty_layout = descriptor_set_layout_create(context, bindings);
        s_globals->empty_descriptor_set = descriptor_set_allocate(context, s_globals->global_data_dp, empty_layout);
        descriptor_set_layout_destroy(context, empty_layout);
    }

    // linear sampler 2d
    // TODO: how should we do num mips? Can't base it off of viking room. Set to 10 for now
    {
        s_globals->linear_sampler_2d = sampler_create(context, 10);
    }

    {
       s_globals->frame_render_data_buffer = buffer_create(context, BufferType::kUniformBuffer, sizeof(frame_render_data_t));
       buffer_update(context, s_globals->frame_render_data_buffer, context.graphics_command_pool, &s_globals->frame_render_data);
    }

    // write to the global, frame
    {
        descriptor_sets_writes_t descriptor_sets_writes;
        descriptor_sets_writes_add_sampler(descriptor_sets_writes, s_globals->global_ds, s_globals->linear_sampler_2d, 0, 0, 1);
        descriptor_sets_writes_add_uniform_buffer(descriptor_sets_writes, s_globals->frame_ds, s_globals->frame_render_data_buffer, 0, 0, 0, 1);
        descriptor_sets_write(context, descriptor_sets_writes);
    }

    // frames
    s_globals->in_flight_frames.resize(MAX_NUM_FRAMES_IN_FLIGHT);
    for (i32 i = 0; i < MAX_NUM_FRAMES_IN_FLIGHT; i++)
    {
        s_globals->in_flight_frames[i] = frame_create(context);
    }
}

void renderer_globals_destroy(context_t& context)
{
    for (i32 i = 0; i < MAX_NUM_FRAMES_IN_FLIGHT; i++)
	{
        frame_destroy(context, s_globals->in_flight_frames[i]);
	}

    descriptor_set_layout_destroy(context, s_globals->global_data_ds_layout);
    descriptor_set_layout_destroy(context, s_globals->frame_data_ds_layout);
    descriptor_set_layout_destroy(context, s_globals->material_data_ds_layout);
    descriptor_set_layout_destroy(context, s_globals->mesh_instance_render_data_ds_layout);

    descriptor_pool_destroy(context, s_globals->frame_data_dp);
    descriptor_pool_destroy(context, s_globals->material_data_dp);
    descriptor_pool_destroy(context, s_globals->global_data_dp);

    sampler_destroy(context, s_globals->linear_sampler_2d);

    buffer_destroy(context, s_globals->frame_render_data_buffer);

    for(i32 i = 0; i < (i32)s_globals->loaded_mesh_render_data.size(); i++)
    {
        buffer_destroy(context, s_globals->loaded_mesh_render_data[i]->vertex_buffer);
        buffer_destroy(context, s_globals->loaded_mesh_render_data[i]->index_buffer);
    }

    render_pass_destroy(context, s_globals->main_draw_render_pass);
    delete s_globals;
}


renderer_globals_t* get_renderer_globals()
{
    return s_globals;
}

VkPrimitiveTopology mesh_render_data_get_topology(mesh_id_t mesh_id)
{
    i32 data_index = -1;
    for(i32 i = 0; i < (i32)get_renderer_globals()->loaded_mesh_render_data.size(); i++)
    {
        if(get_renderer_globals()->loaded_mesh_render_data[i]->mesh_id == mesh_id)
        {
            data_index = i;
            break;
        }
    }

    ASSERT(-1 != data_index);

    return get_renderer_globals()->loaded_mesh_render_data[data_index]->topology;
}

pipeline_vertex_input_t  mesh_render_data_get_pipeline_vertex_input(mesh_id_t mesh_id)
{
    i32 data_index = -1;
    for(i32 i = 0; i < (i32)get_renderer_globals()->loaded_mesh_render_data.size(); i++)
    {
        if(get_renderer_globals()->loaded_mesh_render_data[i]->mesh_id == mesh_id)
        {
            data_index = i;
            break;
        }
    }

    ASSERT(-1 != data_index);

    return get_renderer_globals()->loaded_mesh_render_data[data_index]->pipeline_vertex_input;
}

instance_draw_id_t frame_get_or_allocate_instance_draw_data(context_t& context, frame_t& frame)
{
    // TODO: we are duplicating the ds layout code here, clean up
    for(i32 i = 0; i < (i32)frame.mesh_instance_render_data.size(); i++)
    {
        if(!frame.mesh_instance_render_data[i].is_assigned)
        {
            frame.mesh_instance_render_data[i].is_assigned = true;
            std::vector<descriptor_set_layout_t> layout(1, get_renderer_globals()->mesh_instance_render_data_ds_layout);
            std::vector<descriptor_set_t> descriptor_set = descriptor_sets_allocate(context, frame.mesh_instance_render_data_descriptor_pool, layout);
            ASSERT(1 == descriptor_set.size());
            frame.mesh_instance_render_data[i].descriptor_set = descriptor_set[0];
            return (instance_draw_id_t)i;
        }
    }

    mesh_instance_render_data_t new_instance_data;
    new_instance_data.data_buffer = buffer_create(context, BufferType::kUniformBuffer, sizeof(instance_draw_data_t));

    std::vector<descriptor_set_layout_t> layout(1, get_renderer_globals()->mesh_instance_render_data_ds_layout);
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

void mesh_instance_render_data_destroy(context_t& context, mesh_instance_render_data_t& data)
{
    buffer_destroy(context, data.data_buffer);
}
