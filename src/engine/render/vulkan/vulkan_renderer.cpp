#include "engine/core/assert.h"
#include "engine/core/config.h"
#include "engine/core/debug.h"
#include "engine/core/file.h"
#include "engine/core/macros.h"
#include "engine/input/input.h"
#include "engine/math/mat44.h"
#include "engine/render/Camera.h"
#include "engine/render/mesh.h"
#include "engine/render/vertex.h"
#include "engine/render/vulkan/vulkan_commands.h"
#include "engine/render/vulkan/vulkan_formats.h"
#include "engine/render/vulkan/vulkan_functions.h"
#include "engine/render/vulkan/vulkan_include.h"
#include "engine/render/vulkan/vulkan_renderer.h"
#include "engine/render/vulkan/vulkan_resource_manager.h"
#include "engine/render/vulkan/vulkan_startup.h"
#include "engine/render/vulkan/vulkan_resources.h"
#include "engine/render/vulkan/vulkan_types.h"
#include "engine/render/window.h"
#include "engine/thirdparty/vulkan/vulkan_core.h"

#include <set>
#include <string>

static context_t* s_context = nullptr;

static
std::vector<VkVertexInputBindingDescription> get_vertex_input_binding_descs(const vertex_pct_t& v)
{
    UNUSED(v);

    std::vector<VkVertexInputBindingDescription> vertex_input_binding_descs;
    vertex_input_binding_descs.resize(1);
    vertex_input_binding_descs[0].binding = 0;
    vertex_input_binding_descs[0].stride = sizeof(v);
    vertex_input_binding_descs[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	return vertex_input_binding_descs;
}

static
std::vector<VkVertexInputAttributeDescription> get_vertex_input_attr_descs(const vertex_pct_t& v)
{
    UNUSED(v);

	std::vector<VkVertexInputAttributeDescription> attr_descs;
	attr_descs.resize(3);

	//pos
	VkVertexInputAttributeDescription pos;
	pos.binding = 0;
	pos.location = 0;
	pos.offset = offsetof(vertex_pct_t, m_pos);
	pos.format = VK_FORMAT_R32G32B32_SFLOAT;

	// color
	VkVertexInputAttributeDescription color;
	color.binding = 0;
	color.location = 1;
	color.offset = offsetof(vertex_pct_t, m_color);
	color.format = VK_FORMAT_R32G32B32A32_SFLOAT;

	// tex coord
	VkVertexInputAttributeDescription uv;
	uv.binding = 0;
	uv.location = 2;
	uv.offset = offsetof(vertex_pct_t, m_uv);
	uv.format = VK_FORMAT_R32G32_SFLOAT;

	attr_descs[0] = pos;
	attr_descs[1] = color;
	attr_descs[2] = uv;

	return attr_descs;
}

static
std::vector<VkVertexInputBindingDescription> mesh_get_vertex_input_binding_descs(mesh_t* mesh)
{
    ASSERT(nullptr != mesh);
	return get_vertex_input_binding_descs(mesh->m_vertices[0]);
}

static
std::vector<VkVertexInputAttributeDescription> mesh_get_vertex_input_attr_descs(mesh_t* mesh)
{
    ASSERT(nullptr != mesh);
	return get_vertex_input_attr_descs(mesh->m_vertices[0]);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////   DMZ DO NOT CROSS
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static mesh_instance_t s_viking_room_mesh_instance;
static mesh_instance_t s_world_axes_mesh_instance;

static
void swapchain_recreate(context_t& context, renderer_globals_t& globals)
{
    if(context.window->was_closed || context.window->width == 0 || context.window->height == 0)
    {
        return;
    }

    vkDeviceWaitIdle(context.device.device_handle);


    context_refresh_swapchain(context);

    // recreate in flight frames
    for (i32 i = 0; i < MAX_NUM_FRAMES_IN_FLIGHT; i++)
	{
        frame_destroy(*s_context, get_renderer_globals()->in_flight_frames[i]);
        get_renderer_globals()->in_flight_frames[i] = frame_create(context);
	}

    mesh_instance_pipeline_refresh(context, s_viking_room_mesh_instance);
    mesh_instance_pipeline_refresh(context, s_world_axes_mesh_instance);
}

static
void update_instance_data(context_t& context, camera_t& camera, frame_t& frame)
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

    {
        s_viking_room_mesh_instance.instance_id = frame_get_or_allocate_instance_draw_data(context, frame);

        instance_draw_data_t instance_draw_data;
        instance_draw_data.mvp = s_viking_room_mesh_instance.transform.model * view_projection;
        frame_update_instance_data(context, frame, s_viking_room_mesh_instance.instance_id, instance_draw_data);
    }

    {
        s_world_axes_mesh_instance.instance_id = frame_get_or_allocate_instance_draw_data(context, frame);

        instance_draw_data_t instance_draw_data;
        instance_draw_data.mvp = s_world_axes_mesh_instance.transform.model * view_projection;
        frame_update_instance_data(context, frame, s_world_axes_mesh_instance.instance_id, instance_draw_data);
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
        //descriptor_sets_writes_add_combined_image_sampler(descriptor_sets_writes, data.descriptor_set, s_viking_room_texture, s_viking_room_sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1, 0, 1);
    }
    descriptor_sets_write(context, descriptor_sets_writes);
}

void renderer_init(window_t* app_window)
{
    ASSERT(nullptr != app_window);

    s_context = context_create(app_window);
    renderer_globals_create(*s_context);

    {
        material_create_info_t mat_create_info;
        mat_create_info.vertex_shader_info = { "shaders/tri-vert.spv", "main" };
        mat_create_info.fragment_shader_info = { "shaders/tri-frag.spv", "main" };

        material_resource_t diffuse_texture = material_load_sampled_texture_resource(*s_context, "textures/viking_room.png", 0, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
        mat_create_info.resources.push_back(diffuse_texture);

        material_t viking_room_material = material_create(*s_context, mat_create_info);
        s_viking_room_mesh_instance.material = viking_room_material;
    }

    {
        material_create_info_t mat_create_info;
        mat_create_info.vertex_shader_info = { "shaders/simple-color-vert.spv", "main" };
        mat_create_info.fragment_shader_info = { "shaders/simple-color-frag.spv", "main" };

        material_t world_axes_material = material_create(*s_context, mat_create_info);
        s_world_axes_mesh_instance.material = world_axes_material;
    }

    // viking room
    {
        // meshes
        mesh_t* viking_room_mesh = mesh_load_from_obj("models/viking_room.obj");
        //mesh_t* cube_mesh = mesh_load_cube();
        s_viking_room_mesh_instance = mesh_instance_create(*s_context, viking_room_mesh->id, s_viking_room_mesh_instance.material);
    }

    // world axes
    {
        mesh_t* world_axes_mesh = mesh_load_axes();
        s_world_axes_mesh_instance = mesh_instance_create(*s_context, world_axes_mesh->id, s_world_axes_mesh_instance.material);
    }
}

void renderer_set_main_camera(camera_t* camera)
{
    ASSERT(nullptr != camera); 
    get_renderer_globals()->main_camera = camera;
}

static
VkResult frame_acquire_next_image(context_t& context, frame_t& frame)
{
	// Acquire an image from the swap chain
	VkResult res = vkAcquireNextImageKHR(context.device.device_handle, 
                                         context.swapchain.handle, 
                                         UINT64_MAX, 
                                         frame.swapchain_image_available_semaphore.handle, 
                                         VK_NULL_HANDLE, 
                                         &frame.swapchain_image_index);
    return res; 
}

static
VkResult frame_present(context_t& context, frame_t& frame)
{
	VkSemaphore wait_semaphores[] = { frame.render_finished_semaphore.handle };

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
mesh_render_data_t* mesh_render_data_find(mesh_id_t mesh_id)
{
    const static i32 INVALID_INDEX = -1;
    i32 render_data_index = INVALID_INDEX;

    for(i32 i = 0; i < get_renderer_globals()->loaded_mesh_render_data.size(); i++)
    {
        if(get_renderer_globals()->loaded_mesh_render_data[i]->mesh_id == mesh_id)
        {
            render_data_index = i;
            break;
        }
    }

    ASSERT(INVALID_INDEX != render_data_index);

    return get_renderer_globals()->loaded_mesh_render_data[render_data_index];
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

        command_buffer_begin_render_pass(command_buffer, get_renderer_globals()->main_draw_render_pass.handle, frame.main_draw_framebuffer.handle, offset, context.swapchain.extent, clear_colors);
            {
                const mesh_render_data_t* mesh_render_data = mesh_render_data_find(s_viking_room_mesh_instance.mesh_id);
                mesh_instance_render_data_t instance_render_data = frame.mesh_instance_render_data[s_viking_room_mesh_instance.instance_id];
                std::vector<VkDescriptorSet> draw_descriptor_sets = { 
                                                                        get_renderer_globals()->global_ds.descriptor_set,
                                                                        get_renderer_globals()->frame_ds.descriptor_set,
                                                                        s_viking_room_mesh_instance.material.descriptor_set.descriptor_set,
                                                                        instance_render_data.descriptor_set.descriptor_set  
                                                                    };
                command_draw_mesh_instance(command_buffer, s_viking_room_mesh_instance, *mesh_render_data, draw_descriptor_sets);
            }

            if(get_renderer_globals()->debug_render)
            {
                const mesh_render_data_t* mesh_render_data = mesh_render_data_find(s_world_axes_mesh_instance.mesh_id);
                mesh_instance_render_data_t instance_render_data = frame.mesh_instance_render_data[s_world_axes_mesh_instance.instance_id];
                std::vector<VkDescriptorSet> draw_descriptor_sets = { 
                                                                        get_renderer_globals()->global_ds.descriptor_set,
                                                                        get_renderer_globals()->frame_ds.descriptor_set,
                                                                        s_world_axes_mesh_instance.material.descriptor_set.descriptor_set,
                                                                        instance_render_data.descriptor_set.descriptor_set  
                                                                    };
                command_draw_mesh_instance(command_buffer, s_world_axes_mesh_instance, *mesh_render_data, draw_descriptor_sets);
            }
        command_buffer_end_render_pass(command_buffer);

        // copy from render target to swap chain for presentation
        command_transition_image_layout(command_buffer, context.swapchain.images[frame.swapchain_image_index], VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1);
        command_copy_image(command_buffer, frame.main_draw_resolve_target.handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
                                           context.swapchain.images[frame.swapchain_image_index], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
                                           context.swapchain.extent.width, context.swapchain.extent.height);
        command_transition_image_layout(command_buffer, context.swapchain.images[frame.swapchain_image_index], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 1);
    }
    command_buffer_end(command_buffer);
}

void renderer_update(f32 ds)
{
    if(input_was_key_pressed(KeyCode::KEY_F1))
    {
        get_renderer_globals()->debug_render = !get_renderer_globals()->debug_render; 
    }

    static f32 time = 0.0f;
    time += ds;

    f32 radius = 3.0f;
    f32 x = radius * cosf(time);
    f32 y = radius * sinf(time);
    f32 z = cosf(x + y);
    vec3 position_ws = make_vec3(x, y, z);

    mat44 rotation = make_rotation_x_deg(cosf(time) * 180.0f);
    rotate_y_deg(rotation, sinf(time) * 180.0f);

    //set_translation(s_viking_room_mesh_instance.transform.model, position_ws);
    //set_rotation(s_viking_room_mesh_instance.transform.model, rotation);
}

void renderer_render_frame()
{
    frame_t& frame = get_renderer_globals()->in_flight_frames[get_renderer_globals()->cur_frame];

	// Wait for frame to finish in case its still in flight, this blocks on CPU
    fence_wait(*s_context, frame.frame_completed_fence);

	// Acquire an image from the swap chain
	VkResult res = frame_acquire_next_image(*s_context, frame); 
	if (res == VK_ERROR_OUT_OF_DATE_KHR)
	{
        swapchain_recreate(*s_context, *get_renderer_globals());
	    return;
	}
	ASSERT(res == VK_SUCCESS || res == VK_SUBOPTIMAL_KHR);

    update_instance_data(*s_context, *get_renderer_globals()->main_camera, frame);

	if (VK_NULL_HANDLE != s_context->swapchain.image_in_flight_fences[frame.swapchain_image_index].handle)
	{
        fence_wait(*s_context, s_context->swapchain.image_in_flight_fences[frame.swapchain_image_index]);
	}

	s_context->swapchain.image_in_flight_fences[frame.swapchain_image_index] = frame.frame_completed_fence;

	// Submit command buffer
	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore wait_semaphores[] = { frame.swapchain_image_available_semaphore.handle };
	VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = wait_semaphores;
	submit_info.pWaitDstStageMask = wait_stages;

    frame_generate_command_buffers(*s_context, frame);
	submit_info.commandBufferCount = frame.command_buffers.size();
	submit_info.pCommandBuffers = frame.command_buffers.data();

	VkSemaphore signal_semaphores[] = { frame.render_finished_semaphore.handle };
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = signal_semaphores;

    fence_reset(*s_context, frame.frame_completed_fence);
	VULKAN_ASSERT(vkQueueSubmit(s_context->device.graphics_queue, 1, &submit_info, frame.frame_completed_fence.handle));

	// Present to screen
    VkResult present_res = frame_present(*s_context, frame);
	if (present_res == VK_ERROR_OUT_OF_DATE_KHR || present_res == VK_SUBOPTIMAL_KHR || s_context->window->was_resized)
	{
        swapchain_recreate(*s_context, *get_renderer_globals());
	}
	else
	{
		VULKAN_ASSERT(res);
	}

    get_renderer_globals()->cur_frame = (get_renderer_globals()->cur_frame + 1) % MAX_NUM_FRAMES_IN_FLIGHT;
}

void renderer_deinit()
{
    vkDeviceWaitIdle(s_context->device.device_handle);

    renderer_globals_destroy(*s_context);

    mesh_instance_destroy(*s_context, s_world_axes_mesh_instance);
    mesh_instance_destroy(*s_context, s_viking_room_mesh_instance);

    context_destroy(s_context);
}

static
VkPrimitiveTopology primitive_topology_to_vk_topology(PrimitiveTopology topology)
{
    switch(topology)
    {
        case PrimitiveTopology::kTriangleList:  return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        case PrimitiveTopology::kLineList:      return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        default: ASSERT(1 == 2); // error out if we haven't put the correct cases here, TODO: use an ERROR("") macro
    }

    return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
}

void renderer_load_mesh(mesh_t* mesh)
{
    for(i32 i = 0; i < (i32)get_renderer_globals()->loaded_mesh_render_data.size(); i++)
    {
        if(get_renderer_globals()->loaded_mesh_render_data[i]->mesh_id == mesh->id)
        {
            return;
        }
    }

    mesh_render_data_t* mesh_render_data = new mesh_render_data_t;
    mesh_render_data->mesh_id = mesh->id;
    mesh_render_data->index_count = (u32)mesh->m_indices.size();
    mesh_render_data->topology = primitive_topology_to_vk_topology(mesh->topology);

    mesh_render_data->vertex_buffer = buffer_create(*s_context, BufferType::kVertexBuffer, mesh_calc_vertex_buffer_size(mesh));
    mesh_render_data->index_buffer = buffer_create(*s_context, BufferType::kIndexBuffer, mesh_calc_index_buffer_size(mesh));
    buffer_update(*s_context, mesh_render_data->vertex_buffer, s_context->graphics_command_pool, mesh->m_vertices.data());
    buffer_update(*s_context, mesh_render_data->index_buffer, s_context->graphics_command_pool, mesh->m_indices.data());

    mesh_render_data->pipeline_vertex_input.input_binding_descs = mesh_get_vertex_input_binding_descs(mesh);
    mesh_render_data->pipeline_vertex_input.input_attr_descs = mesh_get_vertex_input_attr_descs(mesh);
    mesh_render_data->pipeline_vertex_input.vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    mesh_render_data->pipeline_vertex_input.vertex_input_info.vertexBindingDescriptionCount = (u32)mesh_render_data->pipeline_vertex_input.input_binding_descs.size();
    mesh_render_data->pipeline_vertex_input.vertex_input_info.pVertexBindingDescriptions = mesh_render_data->pipeline_vertex_input.input_binding_descs.data();
    mesh_render_data->pipeline_vertex_input.vertex_input_info.vertexAttributeDescriptionCount = (u32)(mesh_render_data->pipeline_vertex_input.input_attr_descs.size());
    mesh_render_data->pipeline_vertex_input.vertex_input_info.pVertexAttributeDescriptions = mesh_render_data->pipeline_vertex_input.input_attr_descs.data();

    get_renderer_globals()->loaded_mesh_render_data.push_back(mesh_render_data);
}

void renderer_unload_mesh(mesh_id_t mesh_id)
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

    if(-1 == data_index) return;
     
    buffer_destroy(*s_context, get_renderer_globals()->loaded_mesh_render_data[data_index]->vertex_buffer);
    buffer_destroy(*s_context, get_renderer_globals()->loaded_mesh_render_data[data_index]->index_buffer);

    delete get_renderer_globals()->loaded_mesh_render_data[data_index];

    get_renderer_globals()->loaded_mesh_render_data.erase(get_renderer_globals()->loaded_mesh_render_data.begin() + data_index);
}
