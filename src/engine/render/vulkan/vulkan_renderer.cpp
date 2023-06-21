#include "engine/core/assert.h"
#include "engine/core/config.h"
#include "engine/core/debug.h"
#include "engine/core/file.h"
#include "engine/core/macros.h"
#include "engine/core/random.h"
#include "engine/core/time.h"
#include "engine/input/input.h"
#include "engine/math/mat44.h"
#include "engine/math/vec3.h"
#include "engine/render/Camera.h"
#include "engine/render/mesh.h"
#include "engine/render/ui/ui.h"
#include "engine/render/vertex.h"
#include "engine/render/vulkan/vulkan_commands.h"
#include "engine/render/vulkan/vulkan_formats.h"
#include "engine/render/vulkan/vulkan_functions.h"
#include "engine/render/vulkan/vulkan_include.h"
#include "engine/render/vulkan/vulkan_renderer.h"
#include "engine/render/vulkan/vulkan_resource_manager.h"
#include "engine/render/vulkan/vulkan_resources.h"
#include "engine/render/vulkan/vulkan_startup.h"
#include "engine/render/vulkan/vulkan_types.h"
#include "engine/render/window.h"
#include "engine/thirdparty/imgui/backends/imgui_impl_win32.h"
#include "engine/thirdparty/imgui/imgui.h"
#include "engine/thirdparty/imgui/backends/imgui_impl_vulkan.h"
#include "engine/thirdparty/vulkan/vulkan_core.h"

#include <set>
#include <string>

static context_t* s_context = nullptr;
static renderer_globals_t* s_globals = nullptr;

struct scene_t
{
    std::vector<mesh_instance_t> mesh_instances;
};
static scene_t s_scene;

static
material_t material_create(context_t& context, material_create_info_t& create_info)
{
    material_t material;
    material.shaders = pipeline_create_shader_stages(context, create_info.vertex_shader_info, create_info.fragment_shader_info);
    material.resources = create_info.resources;

    descriptor_set_layout_bindings_t bindings;
    for(material_resource_t& resource : create_info.resources)
    {
        if(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE == resource.descriptor_info.type)
        {
            descriptor_set_layout_add_binding(bindings, resource.descriptor_info.binding_index, 
                                                        resource.descriptor_info.count, 
                                                        resource.descriptor_info.type, 
                                                        resource.descriptor_info.shader_stages);
        }
    }

    material.descriptor_set_layout = descriptor_set_layout_create(context, bindings);
    material.descriptor_set = descriptor_set_allocate(context, s_globals->material_data_dp, material.descriptor_set_layout);

    descriptor_sets_writes_t descriptor_sets_writes;
    for(material_resource_t& resource : create_info.resources)
    {
        if(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE == resource.descriptor_info.type)
        {
            descriptor_sets_writes_add_sampled_image(descriptor_sets_writes, material.descriptor_set, 
                                                                             resource.descriptor_resource.texture, 
                                                                             VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 
                                                                             resource.descriptor_info.binding_index, 
                                                                             0, //TODO(smerendino): need to support arrays in descriptors
                                                                             resource.descriptor_info.count);
        }
    }
    descriptor_sets_write(context, descriptor_sets_writes);

    return material;
}

static
material_resource_t material_load_sampled_texture_resource(context_t& context, const char* texture_filepath, u32 binding_index, u32 count, VkShaderStageFlagBits shader_stages)
{
    material_resource_t resource = {};
    resource.descriptor_info.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    resource.descriptor_info.binding_index = binding_index;
    resource.descriptor_info.count = count;
    resource.descriptor_info.shader_stages = shader_stages;
    resource.descriptor_resource.texture = texture_create_from_file(context, texture_filepath);
    return resource;
}

// mesh instances
static
void mesh_instance_pipeline_create(context_t& context, mesh_instance_t& mesh_instance)
{
    SM_ASSERT(VK_NULL_HANDLE == mesh_instance.pipeline.pipeline_handle);
    
    const material_t* mat = resource_manager_get_material(mesh_instance.material_id);
    pipeline_shader_stages_t shader_stages = mat->shaders;

    const mesh_render_data_t* mesh_render_data = resource_manager_get_mesh_render_data(mesh_instance.mesh_id);
    pipeline_vertex_input_t vertex_input = mesh_render_data->pipeline_vertex_input;
    VkPrimitiveTopology mesh_topology = mesh_render_data->topology;
    pipeline_input_assembly_t input_assembly = pipeline_create_input_assembly(mesh_topology);
    pipeline_raster_state_t raster_state = pipeline_create_raster_state(VK_POLYGON_MODE_FILL);
    pipeline_viewport_state_t viewport_state;
    pipeline_create_viewport_state(viewport_state, 0.0f, 0.0f, 
                                                   context.swapchain.extent.width, context.swapchain.extent.height, 
                                                   0.0f, 1.0f, 
                                                   0, 0, 
                                                   context.swapchain.extent.width, context.swapchain.extent.width);
    pipeline_multisample_state_t multisample_state = pipeline_create_multisample_state(context.device.max_num_msaa_samples);
    pipeline_depth_stencil_state_t depth_stencil_state = pipeline_create_depth_stencil_state(true, true, VK_COMPARE_OP_LESS, false, 0.0f, 1.0f);
    pipeline_color_blend_state_t color_blend_state;
    pipeline_add_color_blend_attachments(color_blend_state, false, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD, 
                                                                   VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD, 
                                                                   VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT);
    pipeline_create_color_blend_state(color_blend_state, false, VK_LOGIC_OP_COPY, 0.0f, 0.0f, 0.0f, 0.0f);
    pipeline_layout_t pipeline_layout = pipeline_create_layout(context, mesh_instance);
    mesh_instance.pipeline = pipeline_create(context, shader_stages, 
                                                      vertex_input, 
                                                      input_assembly, 
                                                      raster_state, 
                                                      viewport_state, 
                                                      multisample_state, 
                                                      depth_stencil_state, 
                                                      color_blend_state, 
                                                      pipeline_layout, 
                                                      s_globals->main_draw_render_pass);
}

static
name_id_t mesh_instance_get_name_id(const char* name)
{
   return hash(name); 
}

static
mesh_instance_t mesh_instance_create(context_t& context, const char* name, mesh_id_t mesh_id, material_id_t mat_id)
{
    mesh_instance_t mesh_instance;
    mesh_instance.mesh_instance_id = guid_generate();
    mesh_instance.name_id = mesh_instance_get_name_id(name); // TODO(smerendino): We should probalby just have a debug "display name", it would be slow to enfore name uniqueness
    mesh_instance.mesh_id = mesh_id;
    mesh_instance.material_id = mat_id;
    resource_manager_material_acquire(mat_id);
    mesh_instance.transform.model = MAT44_IDENTITY;
    mesh_instance_pipeline_create(context, mesh_instance);
    return mesh_instance;
}

static
mesh_instance_t mesh_instance_create(context_t& context, const char* name, mesh_id_t mesh_id, const char* mat_name)
{
   return mesh_instance_create(context, name, mesh_id, resource_manager_get_material_id(mat_name));
}

static
void mesh_instance_destroy(context_t& context, mesh_instance_t& mesh_instance)
{
    pipeline_destroy(context, mesh_instance.pipeline);
    resource_manager_material_release(context, mesh_instance.material_id);
    resource_manager_mesh_release(context, mesh_instance.mesh_id);
}

static
void mesh_instance_pipeline_refresh(context_t& context, mesh_instance_t& mesh_instance)
{
    if(VK_NULL_HANDLE != mesh_instance.pipeline.pipeline_handle)
    {
        pipeline_destroy(context, mesh_instance.pipeline);
    }
    mesh_instance_pipeline_create(context, mesh_instance);
}

// frames
static
frame_t frame_create(context_t& context)
{
    frame_t frame;
    frame.swapchain_image_index = -1;
    frame.swapchain_image_available_semaphore = semaphore_create(context);
    frame.render_finished_semaphore = semaphore_create(context);
    frame.frame_completed_fence = fence_create(context);
    frame.swapchain_image_acquired_fence = fence_create(context);
    frame.swapchain_copied_semaphore = semaphore_create(context);
    frame.command_buffers = command_buffers_allocate(context, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
    frame.copy_to_backbuffer_command_buffer = command_buffers_allocate(context, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1)[0];

    // render targets / framebuffer
    frame.main_draw_color_target = texture_create_color_target(context, context.swapchain.format, 
            context.swapchain.extent.width, context.swapchain.extent.height, 
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT, 
            context.device.max_num_msaa_samples);

    frame.main_draw_depth_target = texture_create_depth_target(context, context.device.depth_format, 
            context.swapchain.extent.width, context.swapchain.extent.height, 
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT, 
            context.device.max_num_msaa_samples);

    frame.main_draw_resolve_target = texture_create_color_target(context, context.swapchain.format, 
            context.swapchain.extent.width, context.swapchain.extent.height, 
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
            VK_SAMPLE_COUNT_1_BIT);

    std::vector<VkImageView> attachments = { frame.main_draw_color_target.image_view, frame.main_draw_depth_target.image_view, frame.main_draw_resolve_target.image_view };
    frame.main_draw_framebuffer = framebuffer_create(context, s_globals->main_draw_render_pass, attachments, context.swapchain.extent.width, context.swapchain.extent.height, 1);

    std::vector<VkImageView> imgui_attachments = { frame.main_draw_resolve_target.image_view };
    frame.imgui_framebuffer = framebuffer_create(context, s_globals->imgui_render_pass, imgui_attachments, context.swapchain.extent.width, context.swapchain.extent.height, 1);

    // set up frame render data
    {
        frame.frame_render_data_buffer = buffer_create(context, BufferType::kUniformBuffer, sizeof(frame_render_data_t));
        frame.frame_render_data_descriptor_set = descriptor_set_allocate(context, s_globals->frame_render_data_descriptor_pool, s_globals->frame_render_data_descriptor_set_layout);

        descriptor_sets_writes_t descriptor_sets_writes;
        descriptor_sets_writes_add_uniform_buffer(descriptor_sets_writes, 
                                                  frame.frame_render_data_descriptor_set, 
                                                  frame.frame_render_data_buffer, 
                                                  0, 0, 0, 1);
        descriptor_sets_write(context, descriptor_sets_writes);
    }

    // instance render data descriptor pool
    const i32 MAX_SETS = 1024;
    descriptor_pool_sizes_t pool_sizes;
    descriptor_pool_add_size(pool_sizes, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_SETS);
    descriptor_pool_add_size(pool_sizes, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_SETS);
    frame.mesh_instance_render_data_descriptor_pool = descriptor_pool_create(context, pool_sizes, MAX_SETS);

    return frame;
}

static
void frame_destroy(context_t& context, frame_t& frame)
{
    // destroy mesh instance render data
    for (i32 i = 0; i < (i32)frame.mesh_instance_render_data.size(); i++)
    {
        buffer_destroy(context, frame.mesh_instance_render_data[i].data_buffer);
    }

    buffer_destroy(context, frame.frame_render_data_buffer);
    descriptor_pool_destroy(context, frame.mesh_instance_render_data_descriptor_pool);
    framebuffer_destroy(context, frame.main_draw_framebuffer);
    texture_destroy(context, frame.main_draw_color_target);
    texture_destroy(context, frame.main_draw_depth_target);
    texture_destroy(context, frame.main_draw_resolve_target);
    command_buffers_free(context, frame.command_buffers);
    semaphore_destroy(context, frame.swapchain_image_available_semaphore);
    semaphore_destroy(context, frame.render_finished_semaphore);
    fence_destroy(context, frame.frame_completed_fence);
}

static
void frame_update_render_data(context_t& context, frame_t& frame)
{
    buffer_update(context, frame.frame_render_data_buffer, context.graphics_command_pool, &frame.frame_render_data);
}

static
instance_draw_id_t frame_get_or_allocate_instance_draw_data(context_t& context, frame_t& frame)
{
    // TODO(smerendino): we are duplicating the ds layout code here, clean up
    for(i32 i = 0; i < (i32)frame.mesh_instance_render_data.size(); i++)
    {
        if(!frame.mesh_instance_render_data[i].is_assigned)
        {
            frame.mesh_instance_render_data[i].is_assigned = true;
            std::vector<descriptor_set_layout_t> layout(1, s_globals->mesh_instance_render_data_ds_layout);
            std::vector<descriptor_set_t> descriptor_set = descriptor_sets_allocate(context, frame.mesh_instance_render_data_descriptor_pool, layout);
            SM_ASSERT(1 == descriptor_set.size());
            frame.mesh_instance_render_data[i].descriptor_set = descriptor_set[0];
            return (instance_draw_id_t)i;
        }
    }

    mesh_instance_render_data_t new_instance_data;
    new_instance_data.data_buffer = buffer_create(context, BufferType::kUniformBuffer, sizeof(instance_draw_data_t));

    std::vector<descriptor_set_layout_t> layout(1, s_globals->mesh_instance_render_data_ds_layout);
    std::vector<descriptor_set_t> descriptor_set = descriptor_sets_allocate(context, frame.mesh_instance_render_data_descriptor_pool, layout);
    SM_ASSERT(1 == descriptor_set.size());
    new_instance_data.descriptor_set = descriptor_set[0];

    new_instance_data.is_assigned = true;

    frame.mesh_instance_render_data.push_back(new_instance_data);
    return (instance_draw_id_t)(frame.mesh_instance_render_data.size() - 1);
}

static
void frame_update_instance_data(context_t& context, frame_t& frame, instance_draw_id_t instance_id, instance_draw_data_t& instance_draw_data)
{
    buffer_update_data(context, frame.mesh_instance_render_data[instance_id].data_buffer, &instance_draw_data);
}

static
void scene_refresh_pipelines(context_t& context)
{
    for(mesh_instance_t& mesh_instance : s_scene.mesh_instances)
    {
        mesh_instance_pipeline_refresh(context, mesh_instance);
    }
}

static
void swapchain_recreate(context_t& context)
{
    if(context.window->was_closed || context.window->width == 0 || context.window->height == 0)
    {
        return;
    }

    vkDeviceWaitIdle(context.device.device_handle);


    context_refresh_swapchain(context);

    // recreate in flight frames
	descriptor_pool_reset(context, s_globals->frame_render_data_descriptor_pool);
    for (i32 i = 0; i < MAX_NUM_FRAMES_IN_FLIGHT; i++)
	{
        frame_destroy(*s_context, s_globals->in_flight_frames[i]);
        s_globals->in_flight_frames[i] = frame_create(context);
	}

    scene_refresh_pipelines(context);
}

static
void update_mesh_instance_render_data(context_t& context, camera_t& camera, frame_t& frame)
{
    descriptor_pool_reset(context, frame.mesh_instance_render_data_descriptor_pool);

	mat44 view = camera_get_view_tranform(camera);
    f32 aspect = (f32)context.swapchain.extent.width / (f32)context.swapchain.extent.height;
	mat44 projection = create_perspective_projection(45.0, 0.01f, 110.0f, aspect);
    mat44 view_projection = view * projection;

    for(i32 i = 0; i < (i32)frame.mesh_instance_render_data.size(); i++)
    {
        frame.mesh_instance_render_data[i].is_assigned = false;
    }

    // assign instance id for this frame and update instance draw data for each mesh instance
    for(mesh_instance_t& mesh_instance : s_scene.mesh_instances)
    {
        mesh_instance.instance_id = frame_get_or_allocate_instance_draw_data(context, frame);
        instance_draw_data_t instance_draw_data;
        instance_draw_data.mvp = mesh_instance.transform.model * view_projection;
        frame_update_instance_data(context, frame, mesh_instance.instance_id, instance_draw_data);
    }

    // update descriptors
    descriptor_sets_writes_t descriptor_sets_writes;
    for(i32 i = 0; i < (i32)frame.mesh_instance_render_data.size(); i++)
    {
        mesh_instance_render_data_t& data = frame.mesh_instance_render_data[i];
        if(!data.is_assigned)
        {
            continue; 
        }

        descriptor_sets_writes_add_uniform_buffer(descriptor_sets_writes, data.descriptor_set, data.data_buffer, 0, 0, 0, 1);
    }
    descriptor_sets_write(context, descriptor_sets_writes);
}

static
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
                                                 VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 
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
        
        s_globals->global_ds = descriptor_set_allocate(context, s_globals->global_data_dp, s_globals->global_data_ds_layout);
        s_globals->linear_sampler_2d = sampler_create(context, 10);

        descriptor_sets_writes_t descriptor_sets_writes;
        descriptor_sets_writes_add_sampler(descriptor_sets_writes, s_globals->global_ds, s_globals->linear_sampler_2d, 0, 0, 1);
        descriptor_sets_write(context, descriptor_sets_writes);
    }

    // frame data descriptor set layout and pool
    {
        descriptor_set_layout_bindings_t bindings;
        descriptor_set_layout_add_binding(bindings, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL);
        s_globals->frame_render_data_descriptor_set_layout = descriptor_set_layout_create(context, bindings);

        descriptor_pool_sizes_t pool_sizes;
        descriptor_pool_add_size(pool_sizes, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_NUM_FRAMES_IN_FLIGHT);
        s_globals->frame_render_data_descriptor_pool = descriptor_pool_create(context, pool_sizes, MAX_NUM_FRAMES_IN_FLIGHT);
    }

    // material data descriptor pool
    {
        const i32 MAX_SETS = 100;
        descriptor_pool_sizes_t pool_sizes;
        descriptor_pool_add_size(pool_sizes, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, MAX_SETS);
        s_globals->material_data_dp = descriptor_pool_create(context, pool_sizes, MAX_SETS);

        // empty material ds
        descriptor_set_layout_bindings_t bindings;
        descriptor_set_layout_t empty_layout = descriptor_set_layout_create(context, bindings);
        s_globals->empty_descriptor_set = descriptor_set_allocate(context, s_globals->material_data_dp, empty_layout);
        descriptor_set_layout_destroy(context, empty_layout);
    }

    // mesh instance data descriptor set layout
    {
        descriptor_set_layout_bindings_t bindings;
        descriptor_set_layout_add_binding(bindings, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
        s_globals->mesh_instance_render_data_ds_layout = descriptor_set_layout_create(context, bindings);
    }

    // imgui resources
    {
        const i32 MAX_SETS = 1000;
        descriptor_pool_sizes_t pool_sizes;
        descriptor_pool_add_size(pool_sizes, VK_DESCRIPTOR_TYPE_SAMPLER, MAX_SETS);
        descriptor_pool_add_size(pool_sizes, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_SETS);
        descriptor_pool_add_size(pool_sizes, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, MAX_SETS);
        descriptor_pool_add_size(pool_sizes, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, MAX_SETS);
        descriptor_pool_add_size(pool_sizes, VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, MAX_SETS);
        descriptor_pool_add_size(pool_sizes, VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, MAX_SETS);
        descriptor_pool_add_size(pool_sizes, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_SETS);
        descriptor_pool_add_size(pool_sizes, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, MAX_SETS);
        descriptor_pool_add_size(pool_sizes, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, MAX_SETS);
        descriptor_pool_add_size(pool_sizes, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, MAX_SETS);
        descriptor_pool_add_size(pool_sizes, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, MAX_SETS);
        s_globals->imgui_dp = descriptor_pool_create(context, pool_sizes, MAX_SETS);

        render_pass_attachments_t attachments;
        render_pass_add_attachment(attachments, context.swapchain.format, VK_SAMPLE_COUNT_1_BIT, 
                                                 VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
                                                 VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE, 
                                                 VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, 
                                                 0);

        subpasses_t subpasses;
        subpasses_add_attachment_ref(subpasses, 0, SubpassAttachmentRefType::COLOR, 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        subpass_dependencies_t subpass_dependencies;
        subpass_add_dependency(subpass_dependencies, VK_SUBPASS_EXTERNAL, 
                                                     0, 
                                                     VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                                     VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                                     0,
                                                     VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

        s_globals->imgui_render_pass = render_pass_create(context, attachments, subpasses, subpass_dependencies);
    }

    // frames
    s_globals->in_flight_frames.resize(MAX_NUM_FRAMES_IN_FLIGHT);
    for (i32 i = 0; i < MAX_NUM_FRAMES_IN_FLIGHT; i++)
    {
        s_globals->in_flight_frames[i] = frame_create(context);
    }
}

static
void scene_add_mesh_instance(mesh_instance_t& mesh_instance)
{
    s_scene.mesh_instances.push_back(mesh_instance);
}

static
void scene_remove_mesh_instance(const mesh_instance_id_t& mesh_instance_id)
{
    i32 mesh_index = -1;
    for(i32 i = 0; i < (i32)s_scene.mesh_instances.size(); i++)
    {
        if(s_scene.mesh_instances[i].mesh_instance_id == mesh_instance_id)
        {
            mesh_index = i;
            break;
        }
    }

    vkQueueWaitIdle(s_context->device.graphics_queue);

    SM_ASSERT(-1 != mesh_index);
    mesh_instance_destroy(*s_context, s_scene.mesh_instances[mesh_index]);
    s_scene.mesh_instances.erase(s_scene.mesh_instances.begin() + mesh_index);
}

static
void scene_remove_mesh_instance(const name_id_t& name_id)
{
    i32 mesh_index = -1;
    for(i32 i = 0; i < (i32)s_scene.mesh_instances.size(); i++)
    {
        if(s_scene.mesh_instances[i].name_id == name_id)
        {
            mesh_index = i;
            break;
        }
    }

    SM_ASSERT(-1 != mesh_index);
    s_scene.mesh_instances.erase(s_scene.mesh_instances.begin() + mesh_index);
}

static
mesh_instance_id_t scene_create_and_add_mesh_instance(context_t& context, const char* name, mesh_id_t mesh_id, material_id_t mat_id)
{
    mesh_instance_t mesh_instance = mesh_instance_create(context, name, mesh_id, mat_id);
    scene_add_mesh_instance(mesh_instance);
    return mesh_instance.mesh_instance_id;
}

static
mesh_instance_t* scene_get_mesh_instance(mesh_instance_id_t mesh_instance_id)
{
    for(i32 i = 0; i < (i32)s_scene.mesh_instances.size(); i++)
    {
        if(s_scene.mesh_instances[i].mesh_instance_id == mesh_instance_id)
        {
            return &s_scene.mesh_instances[i];
        }
    }

    return nullptr;
}

static
mesh_instance_t* scene_get_mesh_instance(name_id_t name_id)
{
    for(i32 i = 0; i < (i32)s_scene.mesh_instances.size(); i++)
    {
        if(s_scene.mesh_instances[i].name_id == name_id)
        {
            return &s_scene.mesh_instances[i];
        }
    }

    return nullptr;
}

static
void scene_set_mesh_instance_transform(mesh_instance_id_t mesh_instance_id, const transform_t& transform)
{
    mesh_instance_t* mesh_instance = scene_get_mesh_instance(mesh_instance_id);
    SM_ASSERT(nullptr != mesh_instance);
    mesh_instance->transform = transform;
}

static
void renderer_globals_destroy(context_t& context)
{
    for (i32 i = 0; i < MAX_NUM_FRAMES_IN_FLIGHT; i++)
	{
        frame_destroy(context, s_globals->in_flight_frames[i]);
	}

    descriptor_set_layout_destroy(context, s_globals->global_data_ds_layout);
    descriptor_set_layout_destroy(context, s_globals->frame_render_data_descriptor_set_layout);
    descriptor_set_layout_destroy(context, s_globals->mesh_instance_render_data_ds_layout);

    descriptor_pool_destroy(context, s_globals->global_data_dp);
    descriptor_pool_destroy(context, s_globals->frame_render_data_descriptor_pool);
    descriptor_pool_destroy(context, s_globals->material_data_dp);

    sampler_destroy(context, s_globals->linear_sampler_2d);

    render_pass_destroy(context, s_globals->main_draw_render_pass);
    delete s_globals;
}

static std::vector<vec3> s_axis_of_rotation;

static
void add_random_mesh_to_scene(context_t& context)
{
    static i32 counter = 0;

    mesh_id_t viking_room_mesh_id = resource_manager_load_obj_mesh(context, "models/viking_room.obj");
    mesh_id_t cube_mesh_id = resource_manager_get_mesh_id(PrimitiveMeshType::kUnitCube);
    mesh_id_t tetrahedron_mesh_id = resource_manager_get_mesh_id(PrimitiveMeshType::kUnitTetrahedron);
    mesh_id_t octahedron_mesh_id = resource_manager_get_mesh_id(PrimitiveMeshType::kUnitOctahedron);

    material_id_t viking_room_mat_id = resource_manager_get_material_id("viking_room_mat");

    std::string name = "random mesh ";
    name += counter;

    mesh_instance_id_t mesh_instance_id = scene_create_and_add_mesh_instance(context, name.data(), tetrahedron_mesh_id, viking_room_mat_id);
    mesh_instance_t* mi = scene_get_mesh_instance(mesh_instance_id);

    f32 start_dist = random_number_between(1.5f, 10.0f);
    vec3 start_dir = random_unit_vector();
    vec3 start_pos = start_dir * start_dist;

    start_pos = VEC3_ZERO;
    set_translation(mi->transform.model, start_pos);

    vec3 random_axis_of_rotation = random_unit_vector();
    s_axis_of_rotation.push_back(random_axis_of_rotation);

    counter++;
}

static
void remove_most_recent_mesh_from_scene()
{
    if(s_scene.mesh_instances.size() > 1)
    {
        mesh_instance_id_t last_mesh = s_scene.mesh_instances[s_scene.mesh_instances.size() - 1].mesh_instance_id;
        scene_remove_mesh_instance(last_mesh);

        s_axis_of_rotation.erase(s_axis_of_rotation.begin() + s_axis_of_rotation.size() - 1);
    }
}

static mesh_t s_test_mesh;

static
void renderer_load_assets(context_t& context)
{
    {
        material_create_info_t mat_create_info;
        mat_create_info.vertex_shader_info = { "shaders/tri-vert.spv", "main" };
        mat_create_info.fragment_shader_info = { "shaders/tri-frag.spv", "main" };

        material_resource_t diffuse_texture = material_load_sampled_texture_resource(*s_context, "textures/viking_room.png", 0, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
        mat_create_info.resources.push_back(diffuse_texture);

        material_t viking_room_material = material_create(*s_context, mat_create_info);
        resource_manager_track_material_TEMP("viking_room_mat", viking_room_material);
    }

    {
        material_create_info_t mat_create_info;
        mat_create_info.vertex_shader_info = { "shaders/tri-vert.spv", "main" };
        mat_create_info.fragment_shader_info = { "shaders/tri-frag.spv", "main" };

        material_resource_t diffuse_texture = material_load_sampled_texture_resource(*s_context, "textures/gigachad.jpg", 0, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
        mat_create_info.resources.push_back(diffuse_texture);

        material_t viking_room_material = material_create(*s_context, mat_create_info);
        resource_manager_track_material_TEMP("gigachad_mat", viking_room_material);
    }

    {
        material_create_info_t mat_create_info;
        mat_create_info.vertex_shader_info = { "shaders/tri-vert.spv", "main" };
        mat_create_info.fragment_shader_info = { "shaders/tri-frag.spv", "main" };

        material_resource_t diffuse_texture = material_load_sampled_texture_resource(*s_context, "textures/uv-debug.jpg", 0, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
        mat_create_info.resources.push_back(diffuse_texture);

        material_t uv_debug_material = material_create(*s_context, mat_create_info);
        resource_manager_track_material_TEMP("uv_debug_mat", uv_debug_material);
    }

    {
        material_create_info_t mat_create_info;
        mat_create_info.vertex_shader_info = { "shaders/simple-color-vert.spv", "main" };
        mat_create_info.fragment_shader_info = { "shaders/simple-color-frag.spv", "main" };

        material_t simple_vert_color_material = material_create(*s_context, mat_create_info);
        resource_manager_track_material_TEMP("simple_vert_color", simple_vert_color_material);
    }

    // world axes
    {
        mesh_id_t world_axes_mesh_id = resource_manager_get_mesh_id(PrimitiveMeshType::kUnitAxes);
        material_id_t simple_vert_color_mat_id = resource_manager_get_material_id("simple_vert_color");
        scene_create_and_add_mesh_instance(*s_context, "world axes", world_axes_mesh_id, simple_vert_color_mat_id);
    }

    // manually add different meshes I want to lookUnit at, temp code
    {
        mesh_id_t viking_room_mesh_id = resource_manager_load_obj_mesh(context, "models/viking_room.obj");
        mesh_id_t cube_mesh_id = resource_manager_get_mesh_id(PrimitiveMeshType::kUnitCube);
        mesh_id_t tetrahedron_mesh_id = resource_manager_get_mesh_id(PrimitiveMeshType::kUnitTetrahedron);
        mesh_id_t octahedron_mesh_id = resource_manager_get_mesh_id(PrimitiveMeshType::kUnitOctahedron);
        mesh_id_t uv_sphere_mesh_id = resource_manager_get_mesh_id(PrimitiveMeshType::kUnitUvSphere);
        mesh_id_t plane_mesh_id = resource_manager_get_mesh_id(PrimitiveMeshType::kUnitPlane);
        mesh_id_t cone_mesh_id = resource_manager_get_mesh_id(PrimitiveMeshType::kUnitCone);
        mesh_id_t cylinder_mesh_id = resource_manager_get_mesh_id(PrimitiveMeshType::kUnitCylinder);
        mesh_id_t torus_mesh_id = resource_manager_get_mesh_id(PrimitiveMeshType::kUnitTorus);

        material_id_t viking_room_mat_id = resource_manager_get_material_id("viking_room_mat");
        material_id_t debug_mat_id = resource_manager_get_material_id("uv_debug_mat");
        material_id_t gigachad_mat_id = resource_manager_get_material_id("gigachad_mat");

        {
            mesh_instance_id_t mesh_instance_id = scene_create_and_add_mesh_instance(context, "mesh", viking_room_mesh_id, viking_room_mat_id);
            mesh_instance_t* mi = scene_get_mesh_instance(mesh_instance_id);
            mi->transform.model.t.xyz = make_vec3(-3.0f, 0.0f, 0.0f);
        }

        {
            mesh_build_quad_3d(s_test_mesh, make_vec3(0.0f, 0.0f, -3.0f), make_vec3(0.0f, 1.0f, 0.0f), make_vec3(-1.0f, 0.0f, 0.0f), 50.0f, 50.0f, 1);
            mesh_id_t test_id = resource_manager_track_mesh(context, "static/test_mesh", s_test_mesh);
            mesh_instance_id_t mesh_instance_id = scene_create_and_add_mesh_instance(context, "mesh", test_id, gigachad_mat_id);
            mesh_instance_t* mi = scene_get_mesh_instance(mesh_instance_id);
            mi->transform.model.t.xyz = make_vec3(0.0f, 0.0f, 0.0f);
        }

        {
            mesh_instance_id_t mesh_instance_id = scene_create_and_add_mesh_instance(context, "mesh", tetrahedron_mesh_id, debug_mat_id);
            mesh_instance_t* mi = scene_get_mesh_instance(mesh_instance_id);
            mi->transform.model.t.xyz = make_vec3(3.0f, 0.0f, 0.0f);
        }

        {
            mesh_instance_id_t mesh_instance_id = scene_create_and_add_mesh_instance(context, "mesh", cube_mesh_id, debug_mat_id);
            mesh_instance_t* mi = scene_get_mesh_instance(mesh_instance_id);
            mi->transform.model.t.xyz = make_vec3(6.0f, 0.0f, 0.0f);
        }

        {
            mesh_instance_id_t mesh_instance_id = scene_create_and_add_mesh_instance(context, "mesh", octahedron_mesh_id, debug_mat_id);
            mesh_instance_t* mi = scene_get_mesh_instance(mesh_instance_id);
            mi->transform.model.t.xyz = make_vec3(9.0f, 0.0f, 0.0f);
        }

        {
            mesh_instance_id_t mesh_instance_id = scene_create_and_add_mesh_instance(context, "mesh", plane_mesh_id, debug_mat_id);
            mesh_instance_t* mi = scene_get_mesh_instance(mesh_instance_id);
            mi->transform.model.t.xyz = make_vec3(0.0f, 3.0f, 0.0f);
        }

        {
            mesh_instance_id_t mesh_instance_id = scene_create_and_add_mesh_instance(context, "mesh", cylinder_mesh_id, debug_mat_id);
            mesh_instance_t* mi = scene_get_mesh_instance(mesh_instance_id);
            mi->transform.model.t.xyz = make_vec3(3.0f, 3.0f, 0.0f);
        }

        {
            mesh_instance_id_t mesh_instance_id = scene_create_and_add_mesh_instance(context, "mesh", uv_sphere_mesh_id, debug_mat_id);
            mesh_instance_t* mi = scene_get_mesh_instance(mesh_instance_id);
            mi->transform.model.t.xyz = make_vec3(6.0f, 3.0f, 0.0f);
        }

        {
            mesh_instance_id_t mesh_instance_id = scene_create_and_add_mesh_instance(context, "mesh", cone_mesh_id, debug_mat_id);
            mesh_instance_t* mi = scene_get_mesh_instance(mesh_instance_id);
            mi->transform.model.t.xyz = make_vec3(9.0f, 3.0f, 0.0f);
        }

        {
            mesh_instance_id_t mesh_instance_id = scene_create_and_add_mesh_instance(context, "mesh", torus_mesh_id, debug_mat_id);
            mesh_instance_t* mi = scene_get_mesh_instance(mesh_instance_id);
            mi->transform.model.t.xyz = make_vec3(0.0f, 6.0f, 0.0f);
        }

        {
            mesh_t* mesh = new mesh_t;
            mesh->topology = PrimitiveTopology::kTriangleList;
            mesh_build_cylinder(*mesh, make_vec3(0.0f, 0.0f, 0.0f), make_vec3(1.0f, 0.0f, 0.0f), 1.0f, 0.01f, 32);
            mesh_id_t test_cylinder_id = resource_manager_track_mesh(context, "custom_cylinder", *mesh);
            scene_create_and_add_mesh_instance(context, "mesh", test_cylinder_id, debug_mat_id);
        }

        {
            mesh_t* mesh = new mesh_t;
            mesh->topology = PrimitiveTopology::kTriangleList;
            vec3 arrow_origin = make_vec3(2.0f, 2.0f, 3.0f);
            vec3 dir = VEC3_ZERO - arrow_origin;
            mesh_build_arrow_tip_to_origin(*mesh, VEC3_ZERO, make_vec3(1.0f, 1.0f, 1.0f), 5.0f, 0.05f, 0.25f);
            mesh_id_t test_cone_id = resource_manager_track_mesh(context, "custom_cone", *mesh);
            scene_create_and_add_mesh_instance(context, "mesh", test_cone_id, debug_mat_id);
        }
    }
}

static
void check_imgui_vk_result(VkResult result)
{
    SM_VULKAN_ASSERT(result);
}

static
PFN_vkVoidFunction imgui_vulkan_func_loader(const char* function_name, void* user_data)
{
    UNUSED(user_data);
    return vkGetInstanceProcAddr(s_context->instance.handle, function_name);
}

static
void imgui_init()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    
    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(s_context->window->handle);
    ImGui_ImplVulkan_LoadFunctions(imgui_vulkan_func_loader);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = s_context->instance.handle;
    init_info.PhysicalDevice = s_context->device.phys_device_handle;
    init_info.Device = s_context->device.device_handle;
    init_info.QueueFamily = s_context->device.queue_families.graphics_family; 
    init_info.Queue = s_context->device.graphics_queue;
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = s_globals->imgui_dp.descriptor_pool;
    init_info.Subpass = 0;
    init_info.MinImageCount = s_context->swapchain.num_images; 
    init_info.ImageCount = s_context->swapchain.num_images;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator = VK_NULL_HANDLE;
    init_info.CheckVkResultFn = check_imgui_vk_result;
    ImGui_ImplVulkan_Init(&init_info, s_globals->imgui_render_pass.handle);
    
    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != nullptr);

    // Upload Fonts
    {
        VkCommandBuffer command_buffer = command_begin_single_time(*s_context);
        ImGui_ImplVulkan_CreateFontsTexture(command_buffer);
        command_end_single_time(*s_context, command_buffer);
        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }
}

static
void imgui_new_frame()
{
    ImGui_ImplWin32_NewFrame();
    ImGui_ImplVulkan_NewFrame();
    ImGui::NewFrame();
}

void renderer_init(window_t* app_window)
{
    SM_ASSERT(nullptr != app_window);

    s_context = context_create(app_window);
    resource_manager_init(*s_context);
    renderer_globals_create(*s_context);
    renderer_load_assets(*s_context);
    imgui_init();
}

renderer_globals_t* renderer_get_globals()
{
    return s_globals;
}

void renderer_set_main_camera(camera_t* camera)
{
    SM_ASSERT(nullptr != camera); 
    s_globals->main_camera = camera;
}

static
VkResult frame_acquire_next_image(context_t& context, frame_t& frame)
{
    fence_reset(context, frame.swapchain_image_acquired_fence);

	// Acquire an image from the swap chain
	VkResult res = vkAcquireNextImageKHR(context.device.device_handle, 
                                         context.swapchain.handle, 
                                         UINT64_MAX, 
                                         frame.swapchain_image_available_semaphore.handle, 
                                         frame.swapchain_image_acquired_fence.handle, 
                                         &frame.swapchain_image_index);
    return res; 
}

static
VkResult frame_present(context_t& context, frame_t& frame)
{
	VkSemaphore wait_semaphores[] = { frame.swapchain_copied_semaphore.handle };

	VkPresentInfoKHR present_info = {};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = wait_semaphores;
	VkSwapchainKHR swapchains[] = { context.swapchain.handle };
	present_info.swapchainCount = 1;
	present_info.pSwapchains = swapchains;
	present_info.pImageIndices = &frame.swapchain_image_index;
	present_info.pResults = nullptr;

	VkResult res = vkQueuePresentKHR(context.device.graphics_queue, &present_info);
    return res;
}

static
void frame_generate_command_buffers(context_t& context, frame_t& frame)
{
    VkCommandBuffer command_buffer = frame.command_buffers[0];
    vkResetCommandBuffer(command_buffer, 0);

    command_buffer_begin(command_buffer);
    {
        VkOffset2D offset{ 0, 0 };

        VkClearValue color_target_clear;
        color_target_clear.color = {{0.0f, 0.0f, 0.0f, 1.0f}};

        VkClearValue depth_target_clear;
        depth_target_clear.depthStencil = {1.0f, 0};

        std::vector<VkClearValue> clear_colors;
        clear_colors.push_back(color_target_clear);
        clear_colors.push_back(depth_target_clear);

        // main draw meshes
        command_buffer_begin_render_pass(command_buffer, s_globals->main_draw_render_pass.handle, frame.main_draw_framebuffer.handle, offset, context.swapchain.extent, clear_colors);
            for(mesh_instance_t& mesh_instance : s_scene.mesh_instances)
            {
                const mesh_render_data_t* mesh_render_data = resource_manager_get_mesh_render_data(mesh_instance.mesh_id);
                const material_t* mat = resource_manager_get_material(mesh_instance.material_id);
                mesh_instance_render_data_t instance_render_data = frame.mesh_instance_render_data[mesh_instance.instance_id];
                std::vector<VkDescriptorSet> draw_descriptor_sets = { 
                                                                        s_globals->global_ds.descriptor_set,
                                                                        frame.frame_render_data_descriptor_set.descriptor_set,
                                                                        mat->descriptor_set.descriptor_set,
                                                                        instance_render_data.descriptor_set.descriptor_set  
                                                                    };
                command_draw_mesh_instance(command_buffer, mesh_instance, *mesh_render_data, draw_descriptor_sets);
            }
        command_buffer_end_render_pass(command_buffer);

        // imgui
        ui_render();
        command_buffer_begin_render_pass(command_buffer, s_globals->imgui_render_pass.handle, frame.imgui_framebuffer.handle, offset, context.swapchain.extent);
            ::ImGui::Render();
            ::ImDrawData* draw_data = ::ImGui::GetDrawData();
            ImGui_ImplVulkan_RenderDrawData(draw_data, command_buffer);
        command_buffer_end_render_pass(command_buffer);
    }
    command_buffer_end(command_buffer);
}

static
void mesh_instance_set_material(context_t& context, mesh_instance_t* mesh_instance, material_id_t new_mat_id)
{
    SM_ASSERT(nullptr != mesh_instance);
    if (mesh_instance->material_id == new_mat_id)
    {
        return;
    }

    resource_manager_material_release(context, mesh_instance->material_id);
    resource_manager_material_acquire(new_mat_id);
    mesh_instance->material_id = new_mat_id;
}

void renderer_update(f32 ds)
{
    imgui_new_frame();

    name_id_t world_axes_name_id = mesh_instance_get_name_id("world axes");
    material_id_t uv_debug_mat_id = resource_manager_get_material_id("uv_debug_mat");
    material_id_t viking_room_mat_id = resource_manager_get_material_id("viking_room_mat");

    if(input_was_key_pressed(KeyCode::KEY_F1))
    {
        s_globals->debug_render = !s_globals->debug_render; 
    }

    if(input_was_key_pressed(KeyCode::KEY_F2))
    {
        mat44 view = camera_get_view_tranform(*s_globals->main_camera);
        f32 aspect = (f32)s_context->swapchain.extent.width / (f32)s_context->swapchain.extent.height;
        mat44 projection = create_perspective_projection(45.0, 0.01f, 110.0f, aspect);
        mat44 view_projection = view * projection;
        mesh_t* test = new mesh_t;
        test->topology = PrimitiveTopology::kTriangleList;
        mesh_build_frustum(*test, view_projection);
        mesh_id_t frustum_id = resource_manager_track_mesh(*s_context, "test_frustum", *test);
        scene_create_and_add_mesh_instance(*s_context, "test frustum", frustum_id, uv_debug_mat_id);
    }

    if(input_was_key_pressed(KeyCode::KEY_UPARROW))
    {
        //add_random_mesh_to_scene(*s_context);
    }

    if(input_was_key_pressed(KeyCode::KEY_DOWNARROW))
    {
        //remove_most_recent_mesh_from_scene();
    }

    frame_t& frame = s_globals->in_flight_frames[s_globals->cur_frame];
    frame.frame_render_data.time += ds;
    frame.frame_render_data.delta_time_seconds = ds;

    /////////////////////////////////////////////
    // TEMP
    if(s_globals->debug_render)
    {
        ds = 0.0f;
    }

    static f32 pos_degs_per_second = 60.0f;
    static f32 rot_degs_per_second = 90.0f;

    for(i32 i = 0; i < (i32)s_scene.mesh_instances.size(); i++)
    {
        mesh_instance_t& mesh_instance = s_scene.mesh_instances[i]; 
        if(mesh_instance.name_id != world_axes_name_id)
        {
            //mat44 rotation = make_rotation_z_deg(pos_degs_per_second * ds);
            //mat44 rotation = make_rotation_around_axis_deg(s_axis_of_rotation[i - 1], pos_degs_per_second * ds);

            //mesh_instance.transform.model *= rotation;

            //mat44 model_rotation = make_rotation_x_deg(rot_degs_per_second * ds);
            //rotate_y_deg(rotation, rot_degs_per_second * ds);
            //transform_in_model_space(mesh_instance.transform.model, model_rotation);

            //if(s_globals->debug_render)
            //{
            //    mesh_instance_set_material(*s_context, &mesh_instance, uv_debug_mat_id);
            //}
            //else
            //{
            //    mesh_instance_set_material(*s_context, &mesh_instance, viking_room_mat_id);
            //}
        }
    }
    /////////////////////////////////////////////
}

void renderer_render_frame()
{
    frame_t& frame = s_globals->in_flight_frames[s_globals->cur_frame];

	// wait for frame to finish in case its still in flight, this blocks on CPU
    fence_wait(*s_context, frame.frame_completed_fence);

	// acquire an image from the swap chain
	VkResult res = frame_acquire_next_image(*s_context, frame); 
	if (res == VK_ERROR_OUT_OF_DATE_KHR)
	{
        swapchain_recreate(*s_context);
	    return;
	}
	SM_ASSERT(res == VK_SUCCESS || res == VK_SUBOPTIMAL_KHR);

    // update descriptors for the frame and for each mesh instance render data
    frame_update_render_data(*s_context, frame);
    update_mesh_instance_render_data(*s_context, *s_globals->main_camera, frame);

    // wait if the swapchain image is still being used by another in flight frame
	if (VK_NULL_HANDLE != s_context->swapchain.image_in_flight_fences[frame.swapchain_image_index].handle)
	{
        fence_wait(*s_context, s_context->swapchain.image_in_flight_fences[frame.swapchain_image_index]);
	}
	s_context->swapchain.image_in_flight_fences[frame.swapchain_image_index] = frame.frame_completed_fence;

	// Submit frame command buffers
    {
        VkSubmitInfo submit_info = {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.waitSemaphoreCount = 0;
        submit_info.pWaitSemaphores = nullptr;
        submit_info.pWaitDstStageMask = nullptr;

        frame_generate_command_buffers(*s_context, frame);
        submit_info.commandBufferCount = frame.command_buffers.size();
        submit_info.pCommandBuffers = frame.command_buffers.data();

        VkSemaphore signal_semaphores[] = { frame.render_finished_semaphore.handle };
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = signal_semaphores;
        
        SM_VULKAN_ASSERT(vkQueueSubmit(s_context->device.graphics_queue, 1, &submit_info, VK_NULL_HANDLE));
    }

    // Copy to back buffer
    {
        VkSubmitInfo submit_info = {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore wait_semaphores[] = { frame.render_finished_semaphore.handle, frame.swapchain_image_available_semaphore.handle };
        VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT };
        submit_info.waitSemaphoreCount = 2;
        submit_info.pWaitSemaphores = wait_semaphores;
        submit_info.pWaitDstStageMask = wait_stages;

        VkCommandBuffer command_buffer = frame.copy_to_backbuffer_command_buffer;
        vkResetCommandBuffer(command_buffer, 0);

        command_buffer_begin(command_buffer);
        command_transition_image_layout(command_buffer, s_context->swapchain.images[frame.swapchain_image_index], VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1);
        command_copy_image(command_buffer, frame.main_draw_resolve_target.handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
                                           s_context->swapchain.images[frame.swapchain_image_index], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
                                           s_context->swapchain.extent.width, s_context->swapchain.extent.height);
        command_transition_image_layout(command_buffer, s_context->swapchain.images[frame.swapchain_image_index], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 1);
        command_buffer_end(command_buffer);
        
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer;

        VkSemaphore signal_semaphores[] = { frame.swapchain_copied_semaphore.handle };
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = signal_semaphores;
        
        // wait for image acquired fence if needed before submitting this queue
        fence_wait(*s_context, frame.swapchain_image_acquired_fence);

        fence_reset(*s_context, frame.frame_completed_fence);
        SM_VULKAN_ASSERT(vkQueueSubmit(s_context->device.graphics_queue, 1, &submit_info, frame.frame_completed_fence.handle));
    }

	// Present to screen
    VkResult present_res = frame_present(*s_context, frame);
	if (present_res == VK_ERROR_OUT_OF_DATE_KHR || present_res == VK_SUBOPTIMAL_KHR || s_context->window->was_resized)
	{
        swapchain_recreate(*s_context);
	}
	else
	{
		SM_VULKAN_ASSERT(res);
    }

    s_globals->cur_frame = (s_globals->cur_frame + 1) % MAX_NUM_FRAMES_IN_FLIGHT;
}

void renderer_deinit()
{
    vkDeviceWaitIdle(s_context->device.device_handle);

    ::ImGui_ImplVulkan_Shutdown();
    ::ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    for(mesh_instance_t& mesh_instance : s_scene.mesh_instances)
    {
        mesh_instance_destroy(*s_context, mesh_instance);
    }

    renderer_globals_destroy(*s_context);
    resource_manager_deinit(*s_context);

    context_destroy(s_context);
}
