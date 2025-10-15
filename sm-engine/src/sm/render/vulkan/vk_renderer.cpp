#include "sm/render/vulkan/vk_renderer.h"
#include "sm/config.h"
#include "sm/core/bits.h"
#include "sm/core/debug.h"
#include "sm/core/helpers.h"
#include "sm/core/string.h"
#include "sm/math/helpers.h"
#include "sm/math/mat44.h"
#include "sm/render/camera.h"
#include "sm/render/mesh_data.h"
#include "sm/render/shader_compiler.h"
#include "sm/render/ui.h"
#include "sm/render/window.h"
#include "sm/render/vulkan/vk_context.h"
#include "sm/render/vulkan/vk_debug.h"
#include "sm/render/vulkan/vk_debug_draw.h"
#include "sm/render/vulkan/vk_include.h"
#include "sm/render/vulkan/vk_resources.h"

#include "third_party/imgui/imgui.h"
#include "third_party/imgui/imgui_impl_win32.h"
#include "third_party/imgui/imgui_impl_vulkan.h"

using namespace sm;

namespace sm
{
    static const u32 MAX_NUM_MESH_INSTANCES_DEBUG = 1024;
    static const u32 MAX_NUM_MESH_INSTANCES_PER_LEVEL = 1024;
    static const u32 MAX_NUM_MESH_INSTANCES_PER_FRAME = 4096;

    struct level_t 
    {
        string_t* mesh_instance_names[MAX_NUM_MESH_INSTANCES_PER_LEVEL];
        mesh_instances_t mesh_instances;
    };

    struct render_frame_t 
    {
        arena_t* frame_arena;

        // swapchain
        u32	swapchain_image_index = UINT32_MAX;
        VkSemaphore swapchain_image_is_ready_semaphore;

        // frame level resources
        VkFence frame_completed_fence;
        VkSemaphore frame_completed_semaphore;
        VkCommandBuffer frame_command_buffer;

        buffer_t frame_descriptor_buffer;
        VkDescriptorSet frame_descriptor_set;

        texture_t forward_pass_draw_color_multisample_texture;
        texture_t forward_pass_depth_multisample_texture;
        texture_t forward_pass_color_resolve_texture;
        texture_t forward_pass_depth_resolve_texture;
        texture_t post_processing_color_texture;

        // mesh instance descriptors
        buffer_t mesh_instance_render_data_staging_buffer;
        VkDescriptorPool mesh_instance_descriptor_pool;
        buffer_t mesh_instance_render_data_buffers[MAX_NUM_MESH_INSTANCES_PER_FRAME];
        mesh_instances_t mesh_instances;

        // gizmo
        VkDescriptorSet gizmo_descriptor_set;

        // post processing
        buffer_t post_processing_params_buffer;
        VkDescriptorSet	post_processing_descriptor_set;

        // infinite grid
        buffer_t infinite_grid_data_buffer;
        VkDescriptorSet infinite_grid_descriptor_set;
    };

    // uniform buffer layouts
    struct frame_render_data_t
    {
        f32 elapsed_time_seconds;
        f32 delta_time_seconds;
    };

    struct infinite_grid_data_t
    {
        mat44_t view_projection;
        mat44_t inverse_view_projection;
        f32 fade_distance;
        f32 major_line_thickness;
        f32 minor_line_thickness;
    };

    struct mesh_instance_render_data_t
    {
        mat44_t mvp;
    };

    struct post_processing_params_t
    {
        u32 texture_width = 0;
        u32 texture_height = 0;
    };

    // gizmo
    enum class gizmo_mode_t : u8
    {
        TRANSLATE,
        ROTATE,
        SCALE
    };

    struct gizmo_t
    {
        gpu_mesh_t translate_tool_gpu_mesh;
        gpu_mesh_t rotate_tool_gpu_mesh;
        gpu_mesh_t scale_tool_gpu_mesh;
        gizmo_mode_t mode = gizmo_mode_t::TRANSLATE;
    };
}

static bool s_close_window = false;
static render_context_t s_context;

static array_t<render_frame_t> s_render_frames;
static gizmo_t s_gizmo;

static arena_t* s_level_arena = nullptr;
static level_t* s_current_level = nullptr;

static camera_t s_main_camera;
static f32 s_elapsed_time_seconds = 0.0f;
static f32 s_delta_time_seconds = 0.0f;
static u64 s_cur_frame_number = 0;
static u8 s_cur_render_frame_index = 0;

// ui
static mesh_instance_id_t s_selected_mesh_instance_id = INVALID_MESH_INSTANCE_ID;

array_t<collect_mesh_instances_cb_t> s_collect_mesh_instances_cbs;

void level_init(arena_t* arena, level_t* level)
{
    size_t mesh_instance_capacity = MAX_NUM_MESH_INSTANCES_PER_LEVEL;
    memset(level->mesh_instance_names, 0, sizeof(string_t*) * mesh_instance_capacity);
    mesh_instances_init(arena, &level->mesh_instances, mesh_instance_capacity);
}

static void imgui_check_vulkan_result(VkResult result)
{
    SM_VULKAN_ASSERT(result);
}

static PFN_vkVoidFunction imgui_vulkan_func_loader(const char* functionName, void* userData)
{
	VkInstance instance = *((VkInstance*)userData);
    return vkGetInstanceProcAddr(instance, functionName);
}

static void render_frames_init_render_targets()
{
    for(size_t i = 0; i < s_render_frames.cur_size; i++)
    {
        render_frame_t& frame = s_render_frames[i];

        texture_init(s_context, 
                     frame.forward_pass_draw_color_multisample_texture, 
                     s_context.default_color_format, 
                     VkExtent3D(s_context.swapchain.extent.width, s_context.swapchain.extent.height, 1), 
                     VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, 
                     VK_IMAGE_ASPECT_COLOR_BIT, 
                     s_context.max_msaa_samples, 
                     false);

        texture_init(s_context,
                     frame.forward_pass_depth_multisample_texture, 
                     s_context.default_depth_format, 
                     VkExtent3D(s_context.swapchain.extent.width, s_context.swapchain.extent.height, 1), 
                     VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
                     VK_IMAGE_ASPECT_DEPTH_BIT, 
                     s_context.max_msaa_samples, 
                     false);

        texture_init(s_context, 
                     frame.forward_pass_color_resolve_texture, 
                     s_context.default_color_format, 
                     VkExtent3D(s_context.swapchain.extent.width, s_context.swapchain.extent.height, 1), 
                     VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT, 
                     VK_IMAGE_ASPECT_COLOR_BIT, 
                     VK_SAMPLE_COUNT_1_BIT, 
                     false);

        texture_init(s_context, 
                     frame.forward_pass_depth_resolve_texture, 
                     s_context.default_depth_format, 
                     VkExtent3D(s_context.swapchain.extent.width, s_context.swapchain.extent.height, 1), 
                     VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
                     VK_IMAGE_ASPECT_DEPTH_BIT, 
                     VK_SAMPLE_COUNT_1_BIT, 
                     false);

        texture_init(s_context,
                     frame.post_processing_color_texture, 
                     s_context.default_color_format, 
                     VkExtent3D(s_context.swapchain.extent.width, s_context.swapchain.extent.height, 1), 
                     VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, 
                     VK_IMAGE_ASPECT_COLOR_BIT, 
                     VK_SAMPLE_COUNT_1_BIT, 
                     false);
	}
}

static void render_frames_init()
{
    for(size_t render_frame_index = 0; render_frame_index < s_render_frames.cur_size; render_frame_index++)
    {
        render_frame_t& frame = s_render_frames[render_frame_index];

        // frame arena for per-frame allocations
        {
            frame.frame_arena = arena_init(MiB(1));
        }

		// swapchain image is ready semaphore
        {
            VkSemaphoreCreateInfo create_info{};
            create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            SM_VULKAN_ASSERT(vkCreateSemaphore(s_context.device, &create_info, nullptr, &frame.swapchain_image_is_ready_semaphore));
        }

		// frame completed semaphore
        {
            VkSemaphoreCreateInfo create_info{};
            create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            SM_VULKAN_ASSERT(vkCreateSemaphore(s_context.device, &create_info, nullptr, &frame.frame_completed_semaphore));
        }

		// frame completed fence
        {
            VkFenceCreateInfo create_info{};
            create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
            SM_VULKAN_ASSERT(vkCreateFence(s_context.device, &create_info, nullptr, &frame.frame_completed_fence));
        }

		// frame command buffer
        {
            VkCommandBufferAllocateInfo alloc_info{};
            alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            alloc_info.commandBufferCount = 1;
            alloc_info.commandPool = s_context.graphics_command_pool;
            alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            SM_VULKAN_ASSERT(vkAllocateCommandBuffers(s_context.device, &alloc_info, &frame.frame_command_buffer));
        }

		// frame descriptor set
        {
            VkDescriptorSetAllocateInfo alloc_info{};
            alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            alloc_info.descriptorPool = g_frame_descriptor_pool;
            VkDescriptorSetLayout layouts[] = {
                g_frame_descriptor_set_layout
            };
            alloc_info.pSetLayouts = layouts;
            alloc_info.descriptorSetCount = ARRAY_LEN(layouts);
            vkAllocateDescriptorSets(s_context.device, &alloc_info, &frame.frame_descriptor_set);
        }

		// frame uniform buffer
        {
            buffer_init(s_context, 
                        frame.frame_descriptor_buffer, 
                        sizeof(frame_render_data_t), 
                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        }

		// frame command buffer
        {
            VkCommandBufferAllocateInfo alloc_info{};
            alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            alloc_info.commandBufferCount = 1;
            alloc_info.commandPool = s_context.graphics_command_pool;
            alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            SM_VULKAN_ASSERT(vkAllocateCommandBuffers(s_context.device, &alloc_info, &frame.frame_command_buffer));
        }

        // mesh instance descriptor pool
        {
            VkDescriptorPoolCreateInfo create_info{};
            create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            create_info.maxSets = 100;
            VkDescriptorPoolSize pool_sizes[] = {
                { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100 }
            };
            create_info.poolSizeCount = ARRAY_LEN(pool_sizes);
            create_info.pPoolSizes = pool_sizes;
            SM_VULKAN_ASSERT(vkCreateDescriptorPool(s_context.device, &create_info, nullptr, &frame.mesh_instance_descriptor_pool));
        }

        // mesh instance render data buffers
        {
            buffer_init(s_context, 
                        frame.mesh_instance_render_data_staging_buffer,
                        sizeof(mesh_instance_render_data_t) * MAX_NUM_MESH_INSTANCES_PER_FRAME,
                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

            for(u32 mesh_instance_index = 0; mesh_instance_index < MAX_NUM_MESH_INSTANCES_PER_FRAME; ++mesh_instance_index)
            {
                buffer_init(s_context, 
                            frame.mesh_instance_render_data_buffers[mesh_instance_index], 
                            sizeof(mesh_instance_render_data_t), 
                            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            }
        }

		// post processing descriptor set
        {
            buffer_init(s_context, 
                        frame.post_processing_params_buffer, 
                        sizeof(post_processing_params_t), 
                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            VkDescriptorSetAllocateInfo alloc_info{};
            alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            alloc_info.descriptorPool = g_frame_descriptor_pool;
            VkDescriptorSetLayout set_layouts[] = {
                g_post_process_descriptor_set_layout
            };
            alloc_info.pSetLayouts = set_layouts;
            alloc_info.descriptorSetCount = ARRAY_LEN(set_layouts);
            SM_VULKAN_ASSERT(vkAllocateDescriptorSets(s_context.device, &alloc_info, &frame.post_processing_descriptor_set));
        }

        // infinite grid
        {
            // uniform buffer
            {
                buffer_init(s_context, 
                            frame.infinite_grid_data_buffer, 
                            sizeof(infinite_grid_data_t), 
                            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            }

            // descriptor set
            {
                VkDescriptorSetAllocateInfo alloc_info{};
                alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                alloc_info.descriptorPool = g_frame_descriptor_pool;

                VkDescriptorSetLayout descriptor_set_layouts[] = {
                    g_infinite_grid_descriptor_set_layout
                };
                alloc_info.descriptorSetCount = ARRAY_LEN(descriptor_set_layouts);
                alloc_info.pSetLayouts = descriptor_set_layouts;

                SM_VULKAN_ASSERT(vkAllocateDescriptorSets(s_context.device, &alloc_info, &frame.infinite_grid_descriptor_set));
            }
        }
    }

	render_frames_init_render_targets();
}

static void render_frames_refresh_render_targets()
{
    for(size_t i = 0; i < s_render_frames.cur_size; i++)
    {
        render_frame_t& frame = s_render_frames[i];
        texture_release(s_context, frame.forward_pass_draw_color_multisample_texture);
        texture_release(s_context, frame.forward_pass_depth_multisample_texture);
        texture_release(s_context, frame.forward_pass_color_resolve_texture);
        texture_release(s_context, frame.forward_pass_depth_resolve_texture);
        texture_release(s_context, frame.post_processing_color_texture);
	}

	render_frames_init_render_targets();
}

static void refresh_pipelines()
{
    pipelines_recreate(s_context);
}

static void refresh_swapchain()
{
	swapchain_init(s_context);
	render_frames_refresh_render_targets();
    refresh_pipelines();
}

void renderer_window_msg_handler(window_msg_type_t msg_type, u64 msg_data, void* user_args)
{
    if(msg_type == window_msg_type_t::CLOSE_WINDOW)
    {
        s_close_window = true;
    }
}

static void gizmo_collect_mesh_instances(arena_t* frame_allocator, mesh_instances_t* frame_mesh_instances)
{
    if (s_selected_mesh_instance_id == INVALID_MESH_INSTANCE_ID)
    {
        return;
    }

    mesh_instances_t mesh_instances;
    mesh_instances_init(frame_allocator, &mesh_instances, 3);

    transform_t selected_mesh_instance_transform = s_current_level->mesh_instances.transforms[s_selected_mesh_instance_id];

    gpu_mesh_t* gizmo_mesh_to_render = nullptr;
    switch (s_gizmo.mode)
    {
		case gizmo_mode_t::TRANSLATE: gizmo_mesh_to_render = &s_gizmo.translate_tool_gpu_mesh; break;
		case gizmo_mode_t::ROTATE: gizmo_mesh_to_render = &s_gizmo.rotate_tool_gpu_mesh; break;
		case gizmo_mode_t::SCALE: gizmo_mesh_to_render = &s_gizmo.scale_tool_gpu_mesh; break;
    }

    {
		transform_t gizmo_transform;
		set_translation(gizmo_transform.model, selected_mesh_instance_transform.model.tx, selected_mesh_instance_transform.model.ty, selected_mesh_instance_transform.model.tz);

		debug_draw_push_constants_t* gizmo_push_constants = arena_alloc_struct(frame_allocator, debug_draw_push_constants_t);
		gizmo_push_constants->color = to_vec4(color_f32_t::RED);

		push_constants_t push_constants;
		push_constants.data = gizmo_push_constants;
		push_constants.size = sizeof(debug_draw_push_constants_t);

		mesh_instances_add(&mesh_instances, gizmo_mesh_to_render, g_debug_draw_material, push_constants, gizmo_transform, (u32)mesh_instance_flags_t::IS_DEBUG);
    }

    {
		transform_t gizmo_transform;
		rotate_z_degs(gizmo_transform.model, 90.0f);
		set_translation(gizmo_transform.model, selected_mesh_instance_transform.model.tx, selected_mesh_instance_transform.model.ty, selected_mesh_instance_transform.model.tz);

		debug_draw_push_constants_t* gizmo_push_constants = arena_alloc_struct(frame_allocator, debug_draw_push_constants_t);
		gizmo_push_constants->color = to_vec4(color_f32_t::GREEN);

		push_constants_t push_constants;
		push_constants.data = gizmo_push_constants;
		push_constants.size = sizeof(debug_draw_push_constants_t);

		mesh_instances_add(&mesh_instances, gizmo_mesh_to_render, g_debug_draw_material, push_constants, gizmo_transform, (u32)mesh_instance_flags_t::IS_DEBUG);
    }

    {
		transform_t gizmo_transform;
		rotate_y_degs(gizmo_transform.model, -90.0f);
		set_translation(gizmo_transform.model, selected_mesh_instance_transform.model.tx, selected_mesh_instance_transform.model.ty, selected_mesh_instance_transform.model.tz);

		debug_draw_push_constants_t* gizmo_push_constants = arena_alloc_struct(frame_allocator, debug_draw_push_constants_t);
		gizmo_push_constants->color = to_vec4(color_f32_t::BLUE);

		push_constants_t push_constants;
		push_constants.data = gizmo_push_constants;
		push_constants.size = sizeof(debug_draw_push_constants_t);

		mesh_instances_add(&mesh_instances, gizmo_mesh_to_render, g_debug_draw_material, push_constants, gizmo_transform, (u32)mesh_instance_flags_t::IS_DEBUG);
    }
    mesh_instances_append(frame_mesh_instances, &mesh_instances);
}


static void gizmo_init(arena_t* arena)
{
    f32 gizmo_length = 0.75f;
    f32 gizmo_bar_thickness = 0.015f;
    f32 scale_box_thickness = 0.075f;

    // build translate tool mesh
    cpu_mesh_t* translate_mesh = mesh_data_init(arena);
    mesh_data_add_cylinder(translate_mesh, vec3_t::ZERO, vec3_t::WORLD_FORWARD, gizmo_length, gizmo_bar_thickness, 32, color_f32_t::RED);
    mesh_data_add_cone(translate_mesh, { .x = gizmo_length, .y = 0.0f, .z = 0.0f }, vec3_t::WORLD_FORWARD, 0.25f, 0.1f, 32, color_f32_t::RED);
    gpu_mesh_init(s_context, *translate_mesh, s_gizmo.translate_tool_gpu_mesh);

    // build rotate tool mesh
    cpu_mesh_t* rotate_mesh = mesh_data_init(arena);
    mesh_data_add_torus(rotate_mesh, vec3_t::ZERO, vec3_t::WORLD_FORWARD, 0.65f, 0.025f, 32);
    gpu_mesh_init(s_context, *rotate_mesh, s_gizmo.rotate_tool_gpu_mesh);

    // build scale tool mesh
    cpu_mesh_t* scale_mesh = mesh_data_init(arena);
    mesh_data_add_cylinder(scale_mesh, vec3_t::ZERO, vec3_t::WORLD_FORWARD, gizmo_length, gizmo_bar_thickness, 32, color_f32_t::RED);
    mesh_data_add_cube(scale_mesh, { .x = gizmo_length, .y = 0.0f, .z = 0.0f }, scale_box_thickness, 1, color_f32_t::RED);
    gpu_mesh_init(s_context, *scale_mesh, s_gizmo.scale_tool_gpu_mesh);

    renderer_register_collect_mesh_instances_cb(gizmo_collect_mesh_instances);
}

static void ui_build_scene_window()
{
    ImGui::Text("Scene\n");

	ImGui::RadioButton("Translate", (i32*)&s_gizmo.mode, (i32)gizmo_mode_t::TRANSLATE); ImGui::SameLine();
	ImGui::RadioButton("Rotate", (i32*)&s_gizmo.mode, (i32)gizmo_mode_t::ROTATE); ImGui::SameLine();
	ImGui::RadioButton("Scale", (i32*)&s_gizmo.mode, (i32)gizmo_mode_t::SCALE);

    render_frame_t& render_frame = s_render_frames[s_cur_render_frame_index];

    int item_highlighted_idx = -1; // Here we store our highlighted data as an index.
    if (ImGui::BeginListBox("##scene list", ImVec2(-FLT_MIN, 25 * ImGui::GetTextLineHeightWithSpacing())))
    {
        for (int i = 0; i < render_frame.mesh_instances.capacity; i++)
        {
            mesh_instance_id_t cur_id = render_frame.mesh_instances.ids[i];
            if (cur_id == INVALID_MESH_INSTANCE_ID)
            {
                continue;
            }

            if (((u32)render_frame.mesh_instances.flags[i] & (u32)mesh_instance_flags_t::IS_DEBUG) != 0)
            {
                continue;
            }

            bool is_selected = (s_selected_mesh_instance_id == cur_id);
            ImGuiSelectableFlags flags = (item_highlighted_idx == i) ? ImGuiSelectableFlags_Highlight : 0;

            string_t* mesh_instance_name = mesh_instances_look_up_name(cur_id);
            if(mesh_instance_name)
            {
                if (ImGui::Selectable(mesh_instance_name->c_str.data, is_selected, flags))
                    s_selected_mesh_instance_id = render_frame.mesh_instances.ids[i];
            }
            else
            {
                char display_name[32];
                sprintf_s(display_name, "mesh id %i", cur_id);
                if (ImGui::Selectable(display_name, is_selected, flags))
                    s_selected_mesh_instance_id = render_frame.mesh_instances.ids[i];
            }

            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndListBox();
    }
}

void sm::renderer_init(window_t* window)
{	
	arena_t* startup_arena = arena_init(MiB(100));

    window_add_msg_cb(window, renderer_window_msg_handler, nullptr);
    s_collect_mesh_instances_cbs = array_init<collect_mesh_instances_cb_t>(startup_arena, 1024);

    s_context = render_context_init(startup_arena, window);

	shader_compiler_init();
	mesh_data_primitives_init();
    mesh_instances_names_init();
    material_init(s_context);
    debug_draw_init(s_context);
    gizmo_init(startup_arena);

    s_main_camera.world_pos = vec3_t{ .x = 3.0f, .y = 3.0f, .z = 3.0f };
    camera_look_at(s_main_camera, vec3_t::ZERO);

    // imgui
    {
        IMGUI_CHECKVERSION();
		ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

        ImGui::StyleColorsDark();

		HWND hwnd = get_handle<HWND>(s_context.window->handle);

        VkFormat imgui_render_target_format = s_context.default_color_format;
        VkPipelineRenderingCreateInfoKHR pipeline_rendering_create_info{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
            .pNext = nullptr,
            .viewMask = 0,
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &imgui_render_target_format,
            .depthAttachmentFormat = VK_FORMAT_UNDEFINED,
            .stencilAttachmentFormat = VK_FORMAT_UNDEFINED
        };

        ImGui_ImplWin32_Init(hwnd);
        ImGui_ImplVulkan_LoadFunctions(VK_API_VERSION_1_3, imgui_vulkan_func_loader, &s_context.instance);
        ImGui_ImplVulkan_InitInfo init_info{};
        init_info.Instance = s_context.instance;
        init_info.PhysicalDevice = s_context.phys_device;
        init_info.Device = s_context.device;
        init_info.QueueFamily = s_context.queue_indices.graphics;
        init_info.Queue = s_context.graphics_queue;
        init_info.PipelineCache = VK_NULL_HANDLE;
        init_info.DescriptorPool = g_imgui_descriptor_pool;
        init_info.Subpass = 0;
        init_info.MinImageCount = (u32)s_context.swapchain.images.cur_size; 
        init_info.ImageCount = (u32)s_context.swapchain.images.cur_size;
        init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        init_info.Allocator = VK_NULL_HANDLE;
        init_info.CheckVkResultFn = imgui_check_vulkan_result;
        init_info.RenderPass = VK_NULL_HANDLE;
        init_info.UseDynamicRendering = true;
        init_info.PipelineRenderingCreateInfo = pipeline_rendering_create_info;
        ImGui_ImplVulkan_Init(&init_info);

        f32 dpi_scale = ImGui_ImplWin32_GetDpiScaleForHwnd(hwnd);
        debug_printf("Setting ImGui DPI Scale to %f\n", dpi_scale);

        ImFontConfig font_cfg;
        font_cfg.SizePixels = floor(13.0f * dpi_scale);
        io.Fonts->AddFontDefault(&font_cfg);
        
        ImGui::GetStyle().ScaleAllSizes(dpi_scale);

        // Upload Fonts
        ImGui_ImplVulkan_CreateFontsTexture();
	}

	// render frames
	{
		s_render_frames = array_init_sized<render_frame_t>(startup_arena, MAX_NUM_FRAMES_IN_FLIGHT);
		render_frames_init();
	}

	// resources
	{
	}

    ui_set_build_scene_window_callback(ui_build_scene_window);

    s_level_arena = arena_init(MiB(50));
    s_current_level = arena_alloc_struct(s_level_arena, level_t);
    level_init(s_level_arena, s_current_level);

    u32 grid_size = 2;
    f32 spacing = 2.0f;
    for(u32 y = 0; y < grid_size; y++)
    {
        for(u32 x = 0; x < grid_size; x++)
        {
            transform_t initial_transform;
            translate(initial_transform.model, vec3_t(x * spacing, y * spacing, 0.0f));

            push_constants_t push_constants;
            mesh_instance_id_t added_mesh_instance = mesh_instances_add(&s_current_level->mesh_instances, &g_viking_room_mesh, g_viking_room_material, push_constants, initial_transform, (u32)mesh_instance_flags_t::NONE);

            // set debug name
            char name_string[64];
            sprintf_s(name_string, 64, "viking room %i", y* grid_size + x);

            string_t* debug_string_name = arena_alloc_struct(s_level_arena, string_t);
            *debug_string_name = string_init(s_level_arena, strlen(name_string));
            string_set(*debug_string_name, name_string);

            mesh_instances_register_name(added_mesh_instance, debug_string_name);
        }
    }

	debug_printf("Finished initializing vulkan renderer\n");
}

void sm::renderer_begin_frame()
{
	if (ui_was_reload_shaders_requested())
	{
        vkDeviceWaitIdle(s_context.device);
        refresh_pipelines();
	}

    ui_begin_frame();
}

void sm::renderer_update(f32 ds)
{
	s_elapsed_time_seconds += ds;
	s_delta_time_seconds = ds;

	camera_update(s_main_camera, ds);

	// imgui
    if(!s_close_window && !window_is_minimized(s_context.window))
    {
        ::ImGui_ImplWin32_NewFrame();
        ::ImGui_ImplVulkan_NewFrame();
        ::ImGui::NewFrame();
    }

    debug_draw_update();

    sphere_t s;
    s.center = vec3_t(1.0f, 1.0f, 1.0f);
    s.radius = 0.5f;
    color_f32_t red = color_f32_t::RED;
    red.a = 0.75f;
    debug_draw_sphere(s, red, 1);
}

static void setup_new_frame(render_frame_t& render_frame)
{
    VkFence frame_completed_fences[] = {
        render_frame.frame_completed_fence
    };
    SM_VULKAN_ASSERT(vkWaitForFences(s_context.device, ARRAY_LEN(frame_completed_fences), frame_completed_fences, VK_TRUE, UINT64_MAX));

    SM_VULKAN_ASSERT(vkResetFences(s_context.device, ARRAY_LEN(frame_completed_fences), frame_completed_fences));
    SM_VULKAN_ASSERT(vkResetCommandBuffer(render_frame.frame_command_buffer, 0));
    SM_VULKAN_ASSERT(vkResetDescriptorPool(s_context.device, render_frame.mesh_instance_descriptor_pool, 0));

    arena_reset(render_frame.frame_arena);
    
    VkResult swapchain_image_acquisition_result = vkAcquireNextImageKHR(s_context.device, 
                                                                        s_context.swapchain.handle, 
                                                                        UINT64_MAX, 
                                                                        render_frame.swapchain_image_is_ready_semaphore, 
                                                                        VK_NULL_HANDLE, 
                                                                        &render_frame.swapchain_image_index);
    if(swapchain_image_acquisition_result == VK_SUBOPTIMAL_KHR)
    {
        refresh_swapchain();
    }

    // update frame descriptor
    {
        frame_render_data_t frame_data{};
        frame_data.delta_time_seconds = s_delta_time_seconds;
        frame_data.elapsed_time_seconds = s_elapsed_time_seconds;

        buffer_upload_data(s_context, render_frame.frame_descriptor_buffer.buffer, &frame_data, sizeof(frame_render_data_t));

        VkWriteDescriptorSet descriptor_set_write{};
        descriptor_set_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_set_write.pNext = nullptr;
        descriptor_set_write.dstSet = render_frame.frame_descriptor_set;
        descriptor_set_write.dstBinding = 0;
        descriptor_set_write.dstArrayElement = 0;
        descriptor_set_write.descriptorCount = 1;
        descriptor_set_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptor_set_write.pImageInfo = nullptr;

        VkDescriptorBufferInfo buffer_info{};
        buffer_info.buffer = render_frame.frame_descriptor_buffer.buffer;
        buffer_info.offset = 0;
        buffer_info.range = sizeof(frame_render_data_t);
        descriptor_set_write.pBufferInfo = &buffer_info;

        descriptor_set_write.pTexelBufferView = nullptr;

        VkWriteDescriptorSet descriptor_set_writes[] = {
            descriptor_set_write
        };
        vkUpdateDescriptorSets(s_context.device, ARRAY_LEN(descriptor_set_writes), descriptor_set_writes, 0, nullptr);
    }

    // begin command buffer
    VkCommandBufferBeginInfo command_buffer_begin_info{};
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.pNext = nullptr;
    command_buffer_begin_info.flags = 0;
    command_buffer_begin_info.pInheritanceInfo = nullptr;
    SM_VULKAN_ASSERT(vkBeginCommandBuffer(render_frame.frame_command_buffer, &command_buffer_begin_info));
}

static void upload_mesh_instance_data(render_frame_t& render_frame)
{
    size_t num_mesh_instances = 0;
    for(int i = 0; i < render_frame.mesh_instances.capacity; i++)
    {
        if(render_frame.mesh_instances.ids[i] == INVALID_MESH_INSTANCE_ID)
        {
            break; 
        }

        num_mesh_instances++;
    }

    if (num_mesh_instances == 0) return;

    size_t upload_size = sizeof(mesh_instance_render_data_t) * num_mesh_instances;

    // setup cpu side data
    mat44_t view = camera_get_view_transform(s_main_camera);
    mat44_t projection = init_perspective_proj(45.0f, 0.01f, 100.0f, (f32)s_context.swapchain.extent.width / (f32)s_context.swapchain.extent.height);

    array_t<mesh_instance_render_data_t> mesh_instance_render_data_to_upload = array_init_sized<mesh_instance_render_data_t>(render_frame.frame_arena, num_mesh_instances);
    for (int i = 0; i < num_mesh_instances; i++)
    {
        mat44_t mvp = render_frame.mesh_instances.transforms[i].model * view * projection;
        mesh_instance_render_data_t mesh_instance_render_data{
            .mvp = mvp
        };

        mesh_instance_render_data_to_upload[i] = mesh_instance_render_data;
    }

    // transfer from cpu to staging buffer
    void* data = nullptr;
    vkMapMemory(s_context.device,
                render_frame.mesh_instance_render_data_staging_buffer.memory,
                0,
                upload_size,
                0,
                &data);

    memcpy(data, mesh_instance_render_data_to_upload.data, upload_size);

    vkUnmapMemory(s_context.device, render_frame.mesh_instance_render_data_staging_buffer.memory);

    // transfer from staging buffer to fast gpu buffer for each mesh instance
    for (int i = 0; i < num_mesh_instances; i++)
    {
        VkBufferCopy buffer_copy{
            .srcOffset = sizeof(mesh_instance_render_data_t) * i,
            .dstOffset = 0,
            .size = sizeof(mesh_instance_render_data_t)
        };

        vkCmdCopyBuffer(render_frame.frame_command_buffer,
                        render_frame.mesh_instance_render_data_staging_buffer.buffer,
                        render_frame.mesh_instance_render_data_buffers[i].buffer,
                        1,
                        &buffer_copy);
    }

    VkBufferMemoryBarrier memory_barrier{
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .buffer = render_frame.mesh_instance_render_data_staging_buffer.buffer,
        .offset = 0,
        .size = upload_size
    };

    vkCmdPipelineBarrier(render_frame.frame_command_buffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
        0,
        0, nullptr,
        1, &memory_barrier,
        0, nullptr);
}

static void render_mesh_instances(render_frame_t& render_frame, render_pass_t render_pass)
{
    for(int i = 0; i < render_frame.mesh_instances.capacity; i++)
    {
        if(render_frame.mesh_instances.ids[i] == INVALID_MESH_INSTANCE_ID)
        {
            continue;
        }

        mesh_instance_id_t id = render_frame.mesh_instances.ids[i];
        const gpu_mesh_t* mesh = render_frame.mesh_instances.meshes[i];
        const material_t* material = render_frame.mesh_instances.materials[i];
        push_constants_t push_constants = render_frame.mesh_instances.push_constants[i];
        transform_t transform = render_frame.mesh_instances.transforms[i];

        // this material doesn't have a pipeline setup for this render pass, so skip
        if (material->pipelines[(u32)render_pass] == VK_NULL_HANDLE)
        {
            continue;
        }

        string_t* debug_name = mesh_instances_look_up_name(id);
        if(debug_name)
        {
            SCOPED_COMMAND_BUFFER_DEBUG_LABEL(render_frame.frame_command_buffer, debug_name->c_str.data, color_f32_t(0.0f, 0.0f, 1.0f, 1.0f));
        }

        buffer_t& mesh_instance_buffer = render_frame.mesh_instance_render_data_buffers[i];

        // allocate and update mesh instance descriptor set
        VkDescriptorSetAllocateInfo mesh_instance_descriptor_set_alloc_info{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .pNext = nullptr,
            .descriptorPool = render_frame.mesh_instance_descriptor_pool,
            .descriptorSetCount = 1,
            .pSetLayouts = &g_mesh_instance_descriptor_set_layout
        };

        VkDescriptorSet mesh_instance_descriptor_set = VK_NULL_HANDLE;
        SM_VULKAN_ASSERT(vkAllocateDescriptorSets(s_context.device, &mesh_instance_descriptor_set_alloc_info, &mesh_instance_descriptor_set));

        VkDescriptorBufferInfo descriptor_buffer_info{
            .buffer = mesh_instance_buffer.buffer,
            .offset = 0,
            .range = sizeof(mesh_instance_render_data_t)
        };

        VkWriteDescriptorSet write_mesh_instance_descriptor_set{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = mesh_instance_descriptor_set,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pImageInfo = nullptr,
            .pBufferInfo = &descriptor_buffer_info,
            .pTexelBufferView = nullptr
        };

        VkWriteDescriptorSet descriptor_set_writes[] = {
            write_mesh_instance_descriptor_set
        };

        vkUpdateDescriptorSets(s_context.device, ARRAY_LEN(descriptor_set_writes), descriptor_set_writes, 0, nullptr);

        // bind everything needed to draw
        VkBuffer vertex_buffers[] = {
            mesh->vertex_buffer.buffer
        };
        VkDeviceSize offset = 0;
        VkDeviceSize offsets[] = {
            offset
        };
        vkCmdBindVertexBuffers(render_frame.frame_command_buffer, 0, 1, vertex_buffers, offsets);

        vkCmdBindIndexBuffer(render_frame.frame_command_buffer, mesh->index_buffer.buffer, 0, VK_INDEX_TYPE_UINT32);

        VkDescriptorSet descriptor_sets[] = {
            g_global_descriptor_set,
            render_frame.frame_descriptor_set,
            material->descriptor_sets[(u32)render_pass],
            mesh_instance_descriptor_set	
        };
        vkCmdBindDescriptorSets(render_frame.frame_command_buffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                material->pipeline_layouts[(u32)render_pass],
                                0, 
                                ARRAY_LEN(descriptor_sets), descriptor_sets, 
                                0, nullptr);

        vkCmdBindPipeline(render_frame.frame_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, material->pipelines[(u32)render_pass]);

        if (push_constants.size > 0)
        {
            vkCmdPushConstants(render_frame.frame_command_buffer, material->pipeline_layouts[(u32)render_pass], VK_SHADER_STAGE_FRAGMENT_BIT, 0, (u32)push_constants.size, push_constants.data);
        }

        // draw mesh
        vkCmdDrawIndexed(render_frame.frame_command_buffer, (u32)mesh->num_indices, 1, 0, 0, 0);
    }
}

static void forward_pass(render_frame_t& render_frame)
{
    SCOPED_COMMAND_BUFFER_DEBUG_LABEL(render_frame.frame_command_buffer, "Forward Pass", color_gen_random());

    // transition forward pass render targets to attachment optimal 
    {
        VkImageSubresourceRange color_subresource_range{
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        };
        VkImageSubresourceRange depth_subresource_range{
            .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        };
        VkImageMemoryBarrier transition_forward_pass_color_multisample_to_color_attachment_optimal_barrier{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = VK_ACCESS_NONE,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = render_frame.forward_pass_draw_color_multisample_texture.image,
            .subresourceRange = color_subresource_range
        };
        VkImageMemoryBarrier transition_forward_pass_color_resolve_to_color_attachment_optimal_barrier{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = VK_ACCESS_NONE,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = render_frame.forward_pass_color_resolve_texture.image,
            .subresourceRange = color_subresource_range
        };
        VkImageMemoryBarrier transition_forward_pass_depth_multisample_to_depth_stencil_attachment_optimal_barrier{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = VK_ACCESS_NONE,
            .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = render_frame.forward_pass_depth_multisample_texture.image,
            .subresourceRange = depth_subresource_range 
        };
        VkImageMemoryBarrier image_barriers[] = {
            transition_forward_pass_color_multisample_to_color_attachment_optimal_barrier,
			transition_forward_pass_color_resolve_to_color_attachment_optimal_barrier,
			transition_forward_pass_depth_multisample_to_depth_stencil_attachment_optimal_barrier
        };
        vkCmdPipelineBarrier(render_frame.frame_command_buffer,
                             VK_PIPELINE_STAGE_NONE, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                             0,
                             0, nullptr,
                             0, nullptr,
                             ARRAY_LEN(image_barriers), image_barriers);
    }

    // begin render pass
    {
        color_f32_t clear_color(0.0f, 1.0f, 1.0f, 1.0f);

        VkClearValue color_clear_value{
            .color = { .float32 = { clear_color.r, clear_color.g, clear_color.b, clear_color.a } }
        };

        VkClearValue depth_stencil_clear_value{
            .depthStencil = { .depth = 1.0f, .stencil = 0 },
        };

        VkRenderingAttachmentInfo color_multisample_attachment_info{
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.pNext = nullptr,
			.imageView = render_frame.forward_pass_draw_color_multisample_texture.image_view,
			.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT,
			.resolveImageView = render_frame.forward_pass_color_resolve_texture.image_view,
			.resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.clearValue = color_clear_value
        };
        VkRenderingAttachmentInfo color_attachments[] = {
            color_multisample_attachment_info
        };

        VkRenderingAttachmentInfo depth_stencil_multisample_attachment_info{
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.pNext = nullptr,
			.imageView = render_frame.forward_pass_depth_multisample_texture.image_view,
			.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			.resolveMode = VK_RESOLVE_MODE_MIN_BIT,
			.resolveImageView = render_frame.forward_pass_depth_resolve_texture.image_view,
			.resolveImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.clearValue = depth_stencil_clear_value 
        };

        VkRect2D render_area{
            .offset = { .x = 0, .y = 0 },
			.extent = s_context.swapchain.extent
        };

        VkRenderingInfo rendering_info{
			.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
			.pNext = nullptr,
			.flags = 0,
			.renderArea = render_area,
			.layerCount = 1,
			.viewMask = 0,
			.colorAttachmentCount = ARRAY_LEN(color_attachments),
			.pColorAttachments = color_attachments,
			.pDepthAttachment = &depth_stencil_multisample_attachment_info,
			.pStencilAttachment = nullptr 
        };
        vkCmdBeginRendering(render_frame.frame_command_buffer, &rendering_info);
    }

    render_mesh_instances(render_frame, render_pass_t::FORWARD_PASS);

    vkCmdEndRendering(render_frame.frame_command_buffer);
}

static void post_processing_pass(render_frame_t& render_frame)
{
    SCOPED_COMMAND_BUFFER_DEBUG_LABEL(render_frame.frame_command_buffer, "Post Processing", color_gen_random());

    // transition post process storage image to VK_IMAGE_LAYOUT_GENERAL which is needed for a storage image
    {
        VkImageSubresourceRange subresource_range{
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        };
        VkImageMemoryBarrier transition_forward_pass_color_resolve_to_layout_general_barrier{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_GENERAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = render_frame.forward_pass_color_resolve_texture.image,
            .subresourceRange = subresource_range
        };
        VkImageMemoryBarrier transition_post_process_to_layout_general_barrier{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = VK_ACCESS_NONE,
            .dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_GENERAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = render_frame.post_processing_color_texture.image,
            .subresourceRange = subresource_range
        };
        VkImageMemoryBarrier image_barriers[] = {
			transition_forward_pass_color_resolve_to_layout_general_barrier,
			transition_post_process_to_layout_general_barrier
        };
        vkCmdPipelineBarrier(render_frame.frame_command_buffer,
                             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             0,
                             0, nullptr,
                             0, nullptr,
                             ARRAY_LEN(image_barriers), image_barriers);
    }

    // update post process descriptor set
    {
        post_processing_params_t post_process_params{
            .texture_width  = s_context.swapchain.extent.width,
            .texture_height = s_context.swapchain.extent.height
        };
        buffer_upload_data(s_context, render_frame.post_processing_params_buffer.buffer, &post_process_params, sizeof(post_process_params));

        VkDescriptorImageInfo forward_pass_color_storage_image_descriptor_info{
            .sampler = VK_NULL_HANDLE,
            .imageView = render_frame.forward_pass_color_resolve_texture.image_view,
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL
        };

        VkWriteDescriptorSet forward_pass_color_storage_image_descriptor_set_write{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = render_frame.post_processing_descriptor_set,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .pImageInfo = &forward_pass_color_storage_image_descriptor_info,
            .pBufferInfo = nullptr,
            .pTexelBufferView = nullptr
        };

        VkDescriptorImageInfo post_process_storage_image_descriptor_info{
            .sampler = VK_NULL_HANDLE,
            .imageView = render_frame.post_processing_color_texture.image_view,
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL
        };
        VkWriteDescriptorSet post_process_storage_image_descriptor_set_write{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = render_frame.post_processing_descriptor_set,
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .pImageInfo = &post_process_storage_image_descriptor_info,
            .pBufferInfo = nullptr,
            .pTexelBufferView = nullptr
        };

        VkDescriptorBufferInfo post_process_params_buffer_descriptor_info{
            .buffer = render_frame.post_processing_params_buffer.buffer,
            .offset = 0,
            .range = sizeof(post_processing_params_t)
        };
        VkWriteDescriptorSet post_process_params_buffer_write{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = render_frame.post_processing_descriptor_set,
            .dstBinding = 2,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pImageInfo = nullptr,
            .pBufferInfo = &post_process_params_buffer_descriptor_info,
            .pTexelBufferView = nullptr
        };
        

        VkWriteDescriptorSet descriptor_set_writes[] = {
            forward_pass_color_storage_image_descriptor_set_write,
            post_process_storage_image_descriptor_set_write,
            post_process_params_buffer_write
        };
        vkUpdateDescriptorSets(s_context.device, ARRAY_LEN(descriptor_set_writes), descriptor_set_writes, 0, nullptr);
    }

    VkDescriptorSet post_processing_descriptor_sets[] = {
        render_frame.post_processing_descriptor_set,
        render_frame.frame_descriptor_set
    };

    vkCmdBindDescriptorSets(render_frame.frame_command_buffer, 
                            VK_PIPELINE_BIND_POINT_COMPUTE, 
                            g_post_process_pipeline_layout, 
                            0, 
                            ARRAY_LEN(post_processing_descriptor_sets), post_processing_descriptor_sets,
                            0, nullptr);
    vkCmdBindPipeline(render_frame.frame_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, g_post_process_compute_pipeline);
    vkCmdDispatch(render_frame.frame_command_buffer, s_context.swapchain.extent.width >> 3, s_context.swapchain.extent.height >> 3, 1);

    // transition post process storage image to VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIONAL which is needed for imgui render pass
    {
        VkImageSubresourceRange subresource_range{
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        };
        VkImageMemoryBarrier transition_post_process_to_layout_color_attachment_barrier{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
            .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = render_frame.post_processing_color_texture.image,
            .subresourceRange = subresource_range
        };
        vkCmdPipelineBarrier(render_frame.frame_command_buffer,
                             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                             0,
                             0, nullptr,
                             0, nullptr,
                             1, &transition_post_process_to_layout_color_attachment_barrier);
    }
}

static void debug_pass(render_frame_t& render_frame)
{
    SCOPED_COMMAND_BUFFER_DEBUG_LABEL(render_frame.frame_command_buffer, "Debug Pass", color_gen_random());

    {
        VkRenderingAttachmentInfo color_attachment_info{
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.pNext = nullptr,
			.imageView = render_frame.post_processing_color_texture.image_view,
			.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.resolveMode = VK_RESOLVE_MODE_NONE,
			.resolveImageView = VK_NULL_HANDLE,
			.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.clearValue = 0 
        };
        VkRenderingAttachmentInfo color_attachments[] = {
            color_attachment_info
        };

        VkRenderingAttachmentInfo depth_attachment_info{
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.pNext = nullptr,
			.imageView = render_frame.forward_pass_depth_resolve_texture.image_view,
			.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			.resolveMode = VK_RESOLVE_MODE_NONE,
			.resolveImageView = VK_NULL_HANDLE,
			.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.clearValue = 0 
        };

        VkRect2D render_area{
            .offset = { .x = 0, .y = 0 },
			.extent = s_context.swapchain.extent
        };

        VkRenderingInfo rendering_info{
			.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
			.pNext = nullptr,
			.flags = 0,
			.renderArea = render_area,
			.layerCount = 1,
			.viewMask = 0,
			.colorAttachmentCount = ARRAY_LEN(color_attachments),
			.pColorAttachments = color_attachments,
			.pDepthAttachment = &depth_attachment_info,
			.pStencilAttachment = nullptr 
        };
        vkCmdBeginRendering(render_frame.frame_command_buffer, &rendering_info);
    }

    render_mesh_instances(render_frame, render_pass_t::DEBUG_PASS);

    // end gizmo render pass
    vkCmdEndRendering(render_frame.frame_command_buffer);
}

static void imgui_pass(render_frame_t& render_frame)
{
    SCOPED_COMMAND_BUFFER_DEBUG_LABEL(render_frame.frame_command_buffer, "ImGui Pass", color_gen_random());

    ui_render();

    {
        VkRenderingAttachmentInfo color_attachment_info{
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.pNext = nullptr,
			.imageView = render_frame.post_processing_color_texture.image_view,
			.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.resolveMode = VK_RESOLVE_MODE_NONE,
			.resolveImageView = VK_NULL_HANDLE,
			.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.clearValue = 0 
        };
        VkRenderingAttachmentInfo color_attachments[] = {
            color_attachment_info
        };

        VkRect2D render_area{
            .offset = { .x = 0, .y = 0 },
			.extent = s_context.swapchain.extent
        };

        VkRenderingInfo rendering_info{
			.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
			.pNext = nullptr,
			.flags = 0,
			.renderArea = render_area,
			.layerCount = 1,
			.viewMask = 0,
			.colorAttachmentCount = ARRAY_LEN(color_attachments),
			.pColorAttachments = color_attachments,
			.pDepthAttachment = nullptr,
			.pStencilAttachment = nullptr 
        };

        vkCmdBeginRendering(render_frame.frame_command_buffer, &rendering_info);
    }

    ::ImGui::Render();
    ::ImDrawData* draw_data = ::ImGui::GetDrawData();
    ImGui_ImplVulkan_RenderDrawData(draw_data, render_frame.frame_command_buffer);

    vkCmdEndRendering(render_frame.frame_command_buffer);
}

static void present_frame(render_frame_t& render_frame)
{
    SCOPED_COMMAND_BUFFER_DEBUG_LABEL(render_frame.frame_command_buffer, "Present Frame", color_gen_random());

    // copy from color resolve to swapchain
    {
        // transition swapchain image from present to transfer dst
        {
            VkImageSubresourceRange subresource_range{};
            subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            subresource_range.baseMipLevel = 0;
            subresource_range.levelCount = 1;
            subresource_range.baseArrayLayer = 0;
            subresource_range.layerCount = 1;

            VkImageMemoryBarrier post_processing_to_transfer_src_barrier{
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .pNext = nullptr,
                .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = render_frame.post_processing_color_texture.image,
                .subresourceRange = subresource_range
            };

            VkImageMemoryBarrier present_to_transfer_dst_barrier{
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .pNext = nullptr,
                .srcAccessMask = VK_ACCESS_MEMORY_READ_BIT,
                .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = s_context.swapchain.images[render_frame.swapchain_image_index],
                .subresourceRange = subresource_range
            };

            VkImageMemoryBarrier image_memory_barriers[] = {
                post_processing_to_transfer_src_barrier,
                present_to_transfer_dst_barrier
            };

            vkCmdPipelineBarrier(render_frame.frame_command_buffer,
                                 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 0,
                                 0, nullptr,
                                 0, nullptr,
                                 ARRAY_LEN(image_memory_barriers), image_memory_barriers);
        }

        // blit from forward pass color resolve to swapchain
        {
            VkImageSubresourceLayers src_subresource{};
            src_subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            src_subresource.mipLevel = 0;
            src_subresource.baseArrayLayer = 0;
            src_subresource.layerCount = 1;
            VkOffset3D src_offset_min{};
            src_offset_min.x = 0;
            src_offset_min.y = 0;
            src_offset_min.z = 0;
            VkOffset3D src_offset_max{};
            src_offset_max.x = s_context.swapchain.extent.width;
            src_offset_max.y = s_context.swapchain.extent.height;
            src_offset_max.z = 1;

            VkImageSubresourceLayers dst_subresource{};
            dst_subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            dst_subresource.mipLevel = 0;
            dst_subresource.baseArrayLayer = 0;
            dst_subresource.layerCount = 1;
            VkOffset3D dst_offset_min{};
            dst_offset_min.x = 0;
            dst_offset_min.y = 0;
            dst_offset_min.z = 0;
            VkOffset3D dst_offset_max{};
            dst_offset_max.x = s_context.swapchain.extent.width;
            dst_offset_max.y = s_context.swapchain.extent.height;
            dst_offset_max.z = 1;

            VkImageBlit image_blit_region{};
            image_blit_region.srcSubresource = src_subresource;
            image_blit_region.srcOffsets[0] = src_offset_min;
            image_blit_region.srcOffsets[1] = src_offset_max;
            image_blit_region.dstSubresource = dst_subresource;
            image_blit_region.dstOffsets[0] = dst_offset_min;
            image_blit_region.dstOffsets[1] = dst_offset_max;

            vkCmdBlitImage(render_frame.frame_command_buffer,
                           render_frame.post_processing_color_texture.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           s_context.swapchain.images[render_frame.swapchain_image_index], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1, &image_blit_region, 
                           VK_FILTER_LINEAR);
        }

        // transition swapchain image from transfer dst to present
        {
            VkImageMemoryBarrier transfer_dst_to_present_barrier{};
            transfer_dst_to_present_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            transfer_dst_to_present_barrier.pNext = nullptr;
            transfer_dst_to_present_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            transfer_dst_to_present_barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            transfer_dst_to_present_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            transfer_dst_to_present_barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            transfer_dst_to_present_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            transfer_dst_to_present_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            transfer_dst_to_present_barrier.image = s_context.swapchain.images[render_frame.swapchain_image_index];

            VkImageSubresourceRange subresource_range{};
            subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            subresource_range.baseMipLevel = 0;
            subresource_range.levelCount = 1;
            subresource_range.baseArrayLayer = 0;
            subresource_range.layerCount = 1;
            transfer_dst_to_present_barrier.subresourceRange = subresource_range;

            vkCmdPipelineBarrier(render_frame.frame_command_buffer,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
                                 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &transfer_dst_to_present_barrier);
        }
    }

    vkEndCommandBuffer(render_frame.frame_command_buffer);

    // submit frame command buffer
    VkSemaphore command_buffer_submit_wait_semaphores[] = { 
        render_frame.swapchain_image_is_ready_semaphore 
    };

    VkPipelineStageFlags command_buffer_submit_wait_dst_stage_masks[] = { 
        VK_PIPELINE_STAGE_TRANSFER_BIT 
    };

    VkCommandBuffer command_buffers[] = {
        render_frame.frame_command_buffer
    };

    VkSemaphore command_buffer_submit_signal_semaphores[] = {
        render_frame.frame_completed_semaphore
    };

    VkSubmitInfo frame_command_submit_info{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = ARRAY_LEN(command_buffer_submit_wait_semaphores),
        .pWaitSemaphores = command_buffer_submit_wait_semaphores,
        .pWaitDstStageMask = command_buffer_submit_wait_dst_stage_masks,
        .commandBufferCount = ARRAY_LEN(command_buffers),
        .pCommandBuffers = command_buffers,
        .signalSemaphoreCount = ARRAY_LEN(command_buffer_submit_signal_semaphores),
        .pSignalSemaphores = command_buffer_submit_signal_semaphores
    };

    VkSubmitInfo frame_command_submits[] = {
        frame_command_submit_info
    };
    vkQueueSubmit(s_context.graphics_queue, ARRAY_LEN(frame_command_submits), frame_command_submits, render_frame.frame_completed_fence);

    // present swapchain to screen
    VkSemaphore present_wait_semaphores[] = {
        render_frame.frame_completed_semaphore
    };

    VkPresentInfoKHR present_info{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .waitSemaphoreCount = ARRAY_LEN(present_wait_semaphores),
        .pWaitSemaphores = present_wait_semaphores,
        .swapchainCount = 1,
        .pSwapchains = &s_context.swapchain.handle,
        .pImageIndices = &render_frame.swapchain_image_index,
        .pResults = nullptr
    };

    VkResult present_result = vkQueuePresentKHR(s_context.graphics_queue, &present_info);
    if (present_result == VK_SUBOPTIMAL_KHR || present_result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        refresh_swapchain();
    }
    else
    {
        SM_VULKAN_ASSERT(present_result);
    }
}
static void collect_mesh_instances(render_frame_t& render_frame)
{
    mesh_instances_init(render_frame.frame_arena, &render_frame.mesh_instances, MAX_NUM_MESH_INSTANCES_PER_FRAME);
    mesh_instances_append(&render_frame.mesh_instances, &s_current_level->mesh_instances);

    for (int i = 0; i < s_collect_mesh_instances_cbs.cur_size; i++)
    {
        s_collect_mesh_instances_cbs[i](render_frame.frame_arena, &render_frame.mesh_instances);
    }
}

void sm::renderer_render()
{
    if(s_close_window || window_is_minimized(s_context.window))
    {
        return;
    }

    s_cur_frame_number++;
    s_cur_render_frame_index = s_cur_frame_number % MAX_NUM_FRAMES_IN_FLIGHT;
    render_frame_t& render_frame = s_render_frames[s_cur_render_frame_index];

    SCOPED_QUEUE_DEBUG_LABEL(s_context.graphics_queue, "Graphics Queue", color_f32_t(1.0f, 0.0f, 0.0f, 1.0f));

    setup_new_frame(render_frame);
    collect_mesh_instances(render_frame);
    upload_mesh_instance_data(render_frame);
    forward_pass(render_frame);
    post_processing_pass(render_frame);
    debug_pass(render_frame);
    imgui_pass(render_frame);
    present_frame(render_frame);
}

void sm::renderer_register_collect_mesh_instances_cb(collect_mesh_instances_cb_t cb)
{
    array_push(s_collect_mesh_instances_cbs, cb);
}
