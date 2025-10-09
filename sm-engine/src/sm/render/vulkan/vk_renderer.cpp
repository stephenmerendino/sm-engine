#include "sm/render/vulkan/vk_renderer.h"
#include "sm/config.h"
#include "sm/core/bits.h"
#include "sm/core/debug.h"
#include "sm/core/helpers.h"
#include "sm/core/string.h"
#include "sm/math/helpers.h"
#include "sm/math/mat44.h"
#include "sm/render/camera.h"
#include "sm/render/mesh.h"
#include "sm/render/shader_compiler.h"
#include "sm/render/ui.h"
#include "sm/render/window.h"
#include "sm/render/vulkan/vk_context.h"
#include "sm/render/vulkan/vk_include.h"
#include "sm/render/vulkan/vk_debug_draw.h"

#include "third_party/imgui/imgui.h"
#include "third_party/imgui/imgui_impl_win32.h"
#include "third_party/imgui/imgui_impl_vulkan.h"

#pragma warning(push)
#pragma warning(disable:4244)
#define STB_IMAGE_IMPLEMENTATION
#include "third_party/stb/stb_image.h"
#pragma warning(pop)

using namespace sm;

namespace sm
{
    enum class render_pass_t : u64
    {
        FORWARD_PASS,
        DEBUG_PASS,
        NUM_RENDER_PASSES
    };

    struct buffer_t
    {
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
    };

    struct texture_t
    {
        VkImage image = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkImageView image_view = VK_NULL_HANDLE;
        u32 num_mips = 0;
    };

    struct mesh_t 
    {
        buffer_t vertex_buffer;
        buffer_t index_buffer;
        u32 num_indices = 0;
    };

    static const u32 MAX_NUM_MESH_INSTANCES_DEBUG = 1024;
    static const u32 MAX_NUM_MESH_INSTANCES_PER_LEVEL = 1024;
    static const u32 MAX_NUM_MESH_INSTANCES_PER_FRAME = 4096;

    struct material_t
    {
        VkDescriptorSet descriptor_sets[(u32)render_pass_t::NUM_RENDER_PASSES];
        VkPipelineLayout pipeline_layouts[(u32)render_pass_t::NUM_RENDER_PASSES];
        VkPipeline pipelines[(u32)render_pass_t::NUM_RENDER_PASSES];
    };

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
        mesh_t translate_tool_gpu_mesh;
        mesh_t rotate_tool_gpu_mesh;
        mesh_t scale_tool_gpu_mesh;
        gizmo_mode_t mode = gizmo_mode_t::TRANSLATE;
    };

    struct gizmo_push_constants_t
    {
        vec3_t color;
    };
}

static bool s_close_window = false;
static render_context_t s_context;

static VkDescriptorPool s_global_descriptor_pool = VK_NULL_HANDLE;
static VkDescriptorPool s_frame_descriptor_pool = VK_NULL_HANDLE;
static VkDescriptorPool s_material_descriptor_pool = VK_NULL_HANDLE;
static VkDescriptorPool s_imgui_descriptor_pool = VK_NULL_HANDLE;
static VkDescriptorSetLayout s_empty_descriptor_set_layout = VK_NULL_HANDLE;
static VkDescriptorSetLayout s_global_descriptor_set_layout = VK_NULL_HANDLE;
static VkDescriptorSetLayout s_frame_descriptor_set_layout = VK_NULL_HANDLE;
static VkDescriptorSetLayout s_material_descriptor_set_layout = VK_NULL_HANDLE;
static VkDescriptorSetLayout s_mesh_instance_descriptor_set_layout = VK_NULL_HANDLE;
static VkDescriptorSetLayout s_post_process_descriptor_set_layout = VK_NULL_HANDLE;
static VkDescriptorSetLayout s_infinite_grid_descriptor_set_layout = VK_NULL_HANDLE;
static VkDescriptorSet s_empty_descriptor_set = VK_NULL_HANDLE;
static VkDescriptorSet s_global_descriptor_set = VK_NULL_HANDLE;
static VkSampler s_linear_sampler = VK_NULL_HANDLE;

static array_t<render_frame_t> s_render_frames;

static mesh_t s_viking_room_mesh;
static texture_t s_viking_room_diffuse_texture;
static material_t s_viking_room_material;

static gizmo_t s_gizmo;
static material_t s_gizmo_material;

static VkPipelineLayout s_infinite_grid_forward_pass_pipeline_layout = VK_NULL_HANDLE;

static VkPipelineLayout s_post_process_pipeline_layout = VK_NULL_HANDLE;
static VkPipeline s_post_process_compute_pipeline = VK_NULL_HANDLE;

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

static void begin_queue_debug_label(VkQueue queue, const char* label, const color_f32_t& color)
{
    if (!is_running_in_debug())
    {
        return;
    }

    VkDebugUtilsLabelEXT queue_label{
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
        .pNext = nullptr,
        .pLabelName = label,
        .color = { color.r, color.g, color.b, color.a }
    };

    vkQueueBeginDebugUtilsLabelEXT(queue, &queue_label);
}

static void end_queue_debug_label(VkQueue queue)
{
    if(!is_running_in_debug())
    {
        return;
    }

    vkQueueEndDebugUtilsLabelEXT(queue);
}

static void insert_queue_debug_label(VkQueue queue, const char* label, const color_f32_t& color)
{
    if (!is_running_in_debug())
    {
        return;
    }

    VkDebugUtilsLabelEXT queue_label{
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
        .pNext = nullptr,
        .pLabelName = label,
        .color = { color.r, color.g, color.b, color.a }
    };

    vkQueueInsertDebugUtilsLabelEXT(queue, &queue_label);
}

static void begin_command_buffer_debug_label(VkCommandBuffer command_buffer, const char* label, const color_f32_t& color)
{
    if (!is_running_in_debug())
    {
        return;
    }

    VkDebugUtilsLabelEXT queue_label{
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
        .pNext = nullptr,
        .pLabelName = label,
        .color = { color.r, color.g, color.b, color.a }
    };

    vkCmdBeginDebugUtilsLabelEXT(command_buffer, &queue_label);
}

static void end_command_buffer_debug_label(VkCommandBuffer command_buffer)
{
    if(!is_running_in_debug())
    {
        return;
    }

    vkCmdEndDebugUtilsLabelEXT(command_buffer);
}

static void insert_command_buffer_debug_label(VkCommandBuffer command_buffer, const char* label, const color_f32_t& color)
{
    if (!is_running_in_debug())
    {
        return;
    }

    VkDebugUtilsLabelEXT queue_label{
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
        .pNext = nullptr,
        .pLabelName = label,
        .color = { color.r, color.g, color.b, color.a }
    };

    vkCmdInsertDebugUtilsLabelEXT(command_buffer, &queue_label);
}

class auto_queue_debug_label_t
{
public:
    auto_queue_debug_label_t() = delete;
    auto_queue_debug_label_t(VkQueue _queue, const char* label, const color_f32_t& color)
        :queue(_queue)
    {
        begin_queue_debug_label(queue, label, color);
    }

    ~auto_queue_debug_label_t()
    {
        end_queue_debug_label(queue);
    }

    VkQueue queue;
};

class auto_command_buffer_debug_label_t
{
public:
    auto_command_buffer_debug_label_t() = delete;
    auto_command_buffer_debug_label_t(VkCommandBuffer _command_buffer, const char* label, const color_f32_t& color)
        :command_buffer(_command_buffer)
    {
        begin_command_buffer_debug_label(command_buffer, label, color);
    }

    ~auto_command_buffer_debug_label_t()
    {
        end_command_buffer_debug_label(command_buffer);
    }

    VkCommandBuffer command_buffer;
};

#define SCOPED_QUEUE_DEBUG_LABEL(queue, label, color) \
    auto_queue_debug_label_t CONCATENATE(auto_queue_debug_label, __LINE__)(queue, label, color);

#define SCOPED_COMMAND_BUFFER_DEBUG_LABEL(command_buffer, label, color) \
    auto_command_buffer_debug_label_t CONCATENATE(auto_command_buffer_debug_label, __LINE__) (command_buffer, label, color)

static void imgui_check_vulkan_result(VkResult result)
{
    SM_VULKAN_ASSERT(result);
}

static PFN_vkVoidFunction imgui_vulkan_func_loader(const char* functionName, void* userData)
{
	VkInstance instance = *((VkInstance*)userData);
    return vkGetInstanceProcAddr(instance, functionName);
}

static void set_debug_name(VkObjectType type, u64 handle, const char* debug_name)
{
    if (!is_running_in_debug())
    {
        return;
    }

    VkDebugUtilsObjectNameInfoEXT debug_name_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .pNext = nullptr,
        .objectType = type,
        .objectHandle = handle,
        .pObjectName = debug_name 
    };
    SM_VULKAN_ASSERT(vkSetDebugUtilsObjectNameEXT(s_context.device, &debug_name_info));
}

static void upload_buffer_data(VkBuffer dst_buffer, void* src_data, size_t src_data_size)
{
    VkBuffer staging_buffer = VK_NULL_HANDLE;

    VkBufferCreateInfo staging_buffer_create_info{};
    staging_buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    staging_buffer_create_info.pNext = nullptr;
    staging_buffer_create_info.flags = 0;
    staging_buffer_create_info.size = src_data_size;
    staging_buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    staging_buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    u32 queue_families[] = {
        (u32)s_context.queue_indices.graphics
    };
    staging_buffer_create_info.pQueueFamilyIndices = queue_families;
    staging_buffer_create_info.queueFamilyIndexCount = ARRAY_LEN(queue_families);

    SM_VULKAN_ASSERT(vkCreateBuffer(s_context.device, &staging_buffer_create_info, nullptr, &staging_buffer));

    VkMemoryRequirements staging_buffer_mem_requirements{};
    vkGetBufferMemoryRequirements(s_context.device, staging_buffer, &staging_buffer_mem_requirements);

    VkDeviceMemory staging_buffer_memory = VK_NULL_HANDLE;

    // allocate staging memory
    VkMemoryAllocateInfo staging_alloc_info{};
    staging_alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    staging_alloc_info.pNext = nullptr;
    staging_alloc_info.allocationSize = staging_buffer_mem_requirements.size;
    staging_alloc_info.memoryTypeIndex = find_supported_memory_type_index(s_context, staging_buffer_mem_requirements.memoryTypeBits,
                                                                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    SM_VULKAN_ASSERT(vkAllocateMemory(s_context.device, &staging_alloc_info, nullptr, &staging_buffer_memory));
    SM_VULKAN_ASSERT(vkBindBufferMemory(s_context.device, staging_buffer, staging_buffer_memory, 0));

    // map staging memory and memcpy data into it
    void* gpu_staging_memory = nullptr;
    vkMapMemory(s_context.device, staging_buffer_memory, 0, staging_buffer_mem_requirements.size, 0, &gpu_staging_memory);
    memcpy(gpu_staging_memory, src_data, src_data_size);
    vkUnmapMemory(s_context.device, staging_buffer_memory);

    // transfer data to actual buffer
    VkCommandBufferAllocateInfo command_buffer_alloc_info{};
    command_buffer_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_alloc_info.pNext = nullptr;
    command_buffer_alloc_info.commandPool = s_context.graphics_command_pool;
    command_buffer_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_alloc_info.commandBufferCount = 1;

    VkCommandBuffer buffer_copy_command_buffer = VK_NULL_HANDLE;
    SM_VULKAN_ASSERT(vkAllocateCommandBuffers(s_context.device, &command_buffer_alloc_info, &buffer_copy_command_buffer));

    {
        VkCommandBufferBeginInfo buffer_copy_command_buffer_begin_info{};
        buffer_copy_command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        buffer_copy_command_buffer_begin_info.pNext = nullptr;
        buffer_copy_command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        buffer_copy_command_buffer_begin_info.pInheritanceInfo = nullptr;
        SM_VULKAN_ASSERT(vkBeginCommandBuffer(buffer_copy_command_buffer, &buffer_copy_command_buffer_begin_info));

        SCOPED_QUEUE_DEBUG_LABEL(s_context.graphics_queue, "Staging Buffer Upload", color_gen_random());

        VkBufferCopy buffer_copy{};
        buffer_copy.srcOffset = 0;
        buffer_copy.dstOffset = 0;
        buffer_copy.size = src_data_size;

        VkBufferCopy copy_regions[] = {
            buffer_copy
        };
        vkCmdCopyBuffer(buffer_copy_command_buffer, staging_buffer, dst_buffer, ARRAY_LEN(copy_regions), copy_regions);

        vkEndCommandBuffer(buffer_copy_command_buffer);
    }

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = nullptr;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = nullptr;
    submit_info.pWaitDstStageMask = nullptr;
    VkCommandBuffer commands_to_submit[] = {
        buffer_copy_command_buffer
    };
    submit_info.commandBufferCount = ARRAY_LEN(commands_to_submit);
    submit_info.pCommandBuffers = commands_to_submit;
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = nullptr;

    VkSubmitInfo submit_infos[] = {
        submit_info
    };
    SM_VULKAN_ASSERT(vkQueueSubmit(s_context.graphics_queue, ARRAY_LEN(submit_infos), submit_infos, nullptr));
    vkQueueWaitIdle(s_context.graphics_queue);

    vkDestroyBuffer(s_context.device, staging_buffer, nullptr);
    vkFreeCommandBuffers(s_context.device, s_context.graphics_command_pool, ARRAY_LEN(commands_to_submit), commands_to_submit);
}

static u32 calculate_num_mips(u32 width, u32 height, u32 depth)
{
    u32 max_dimension = max(width, max(height, depth));
    return (u32)(std::floor(std::log2(max_dimension)) + 1);
}

static u32 calculate_num_mips(VkExtent3D size)
{
    return calculate_num_mips(size.width, size.height, size.depth);
}

static void texture_init(texture_t& out_texture, VkFormat format, VkExtent3D size, VkImageUsageFlags usage_flags, VkImageAspectFlags image_aspect, VkSampleCountFlagBits sample_count, bool with_mips_enabled)
{
    out_texture.num_mips = with_mips_enabled ? calculate_num_mips(size) : 1;

    // VkImage
    VkImageCreateInfo image_create_info{};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.extent = size;
    image_create_info.mipLevels = out_texture.num_mips;
    image_create_info.arrayLayers = 1;
    image_create_info.format = format;
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_create_info.usage = usage_flags;
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_create_info.samples = sample_count;
    image_create_info.flags = 0;
    SM_VULKAN_ASSERT(vkCreateImage(s_context.device, &image_create_info, nullptr, &out_texture.image));

    // VkDeviceMemory
    VkMemoryRequirements mem_requirements;
    vkGetImageMemoryRequirements(s_context.device, out_texture.image, &mem_requirements);

    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = find_supported_memory_type_index(s_context, mem_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    SM_VULKAN_ASSERT(vkAllocateMemory(s_context.device, &alloc_info, nullptr, &out_texture.memory));

    vkBindImageMemory(s_context.device, out_texture.image, out_texture.memory, 0);

    // VkImageView
    VkImageViewCreateInfo image_view_create_info{};
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.image = out_texture.image;
    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format = format;
    image_view_create_info.subresourceRange.aspectMask = image_aspect;
    image_view_create_info.subresourceRange.baseMipLevel = 0;
    image_view_create_info.subresourceRange.levelCount = out_texture.num_mips;
    image_view_create_info.subresourceRange.baseArrayLayer = 0;
    image_view_create_info.subresourceRange.layerCount = 1;
    SM_VULKAN_ASSERT(vkCreateImageView(s_context.device, &image_view_create_info, nullptr, &out_texture.image_view));
}

static void texture_init_from_file(texture_t& out_texture, const char* filename, bool generate_mips = true)
{
    arena_t* stack_arena = nullptr;
    arena_stack_init(stack_arena, 256);

    // load image pixels
    sm::string_t full_filepath = string_init(stack_arena);
    string_append(full_filepath, TEXTURES_PATH);
    string_append(full_filepath, filename);

    int tex_width;
    int tex_height;
    int tex_channels;
    stbi_uc* pixels = stbi_load(full_filepath.c_str.data, &tex_width, &tex_height, &tex_channels, STBI_rgb_alpha);

    // calc num mips
    u32 num_mips = generate_mips ? calculate_num_mips(tex_width, tex_height, 1) : 1;

    // calc memory needed
    size_t bytes_per_pixel = 4; // 4 because of STBI_rgb_alpha
    VkDeviceSize image_size = tex_width * tex_height * bytes_per_pixel; 

    // image
    {
        VkImageCreateInfo image_create_info{};
        image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_create_info.pNext = nullptr;
        image_create_info.flags = 0;
        image_create_info.imageType = VK_IMAGE_TYPE_2D;
        image_create_info.format = VK_FORMAT_R8G8B8A8_SRGB;
        image_create_info.extent.width = tex_width;
        image_create_info.extent.height = tex_height;
        image_create_info.extent.depth = 1;
        image_create_info.mipLevels = num_mips;
        image_create_info.arrayLayers = 1;
        image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
        image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        image_create_info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        u32 queue_family_indices[] = {
            (u32)s_context.queue_indices.graphics
        };
        image_create_info.queueFamilyIndexCount = ARRAY_LEN(queue_family_indices);
        image_create_info.pQueueFamilyIndices = queue_family_indices;
        image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        SM_VULKAN_ASSERT(vkCreateImage(s_context.device, &image_create_info, nullptr, &out_texture.image));

        set_debug_name(VK_OBJECT_TYPE_IMAGE, (u64)out_texture.image, filename);
    }

    // image memory
    {
        VkMemoryRequirements image_mem_requirements;
        vkGetImageMemoryRequirements(s_context.device, out_texture.image, &image_mem_requirements);

        VkMemoryAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.pNext = nullptr;
        alloc_info.allocationSize = image_mem_requirements.size;
        alloc_info.memoryTypeIndex = find_supported_memory_type_index(s_context, image_mem_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        SM_VULKAN_ASSERT(vkAllocateMemory(s_context.device, &alloc_info, nullptr, &out_texture.memory));

        SM_VULKAN_ASSERT(vkBindImageMemory(s_context.device, out_texture.image, out_texture.memory, 0));
    }

    // allocate command buffer
    VkCommandBufferAllocateInfo command_buffer_alloc_info{};
    command_buffer_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_alloc_info.pNext = nullptr;
    command_buffer_alloc_info.commandPool = s_context.graphics_command_pool;
    command_buffer_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_alloc_info.commandBufferCount = 1;

    VkCommandBuffer command_buffer = VK_NULL_HANDLE;
    SM_VULKAN_ASSERT(vkAllocateCommandBuffers(s_context.device, &command_buffer_alloc_info, &command_buffer));

    // begin command buffer
    VkCommandBufferBeginInfo command_buffer_begin_info{};
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.pNext = nullptr;
    command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    command_buffer_begin_info.pInheritanceInfo = nullptr;
    SM_VULKAN_ASSERT(vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info));

    // staging buffer upload + copy
    {
        VkBuffer staging_buffer = VK_NULL_HANDLE;
        VkDeviceMemory staging_buffer_memory = VK_NULL_HANDLE;

        VkBufferCreateInfo staging_buffer_create_info{};
        staging_buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        staging_buffer_create_info.pNext = nullptr;
        staging_buffer_create_info.flags = 0;
        staging_buffer_create_info.size = image_size;
        staging_buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        staging_buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        u32 queue_family_indices[] = {
            (u32)s_context.queue_indices.graphics
        };
        staging_buffer_create_info.queueFamilyIndexCount = ARRAY_LEN(queue_family_indices);
        staging_buffer_create_info.pQueueFamilyIndices = queue_family_indices;
        SM_VULKAN_ASSERT(vkCreateBuffer(s_context.device, &staging_buffer_create_info, nullptr, &staging_buffer));

        VkMemoryRequirements staging_buffer_mem_requirements;
        vkGetBufferMemoryRequirements(s_context.device, staging_buffer, &staging_buffer_mem_requirements);

        VkMemoryAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.pNext = nullptr;
        alloc_info.allocationSize = staging_buffer_mem_requirements.size;
        alloc_info.memoryTypeIndex = find_supported_memory_type_index(s_context, staging_buffer_mem_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        SM_VULKAN_ASSERT(vkAllocateMemory(s_context.device, &alloc_info, nullptr, &staging_buffer_memory));

        SM_VULKAN_ASSERT(vkBindBufferMemory(s_context.device, staging_buffer, staging_buffer_memory, 0));

        void* mapped_memory = nullptr;
        SM_VULKAN_ASSERT(vkMapMemory(s_context.device, staging_buffer_memory, 0, image_size, 0, &mapped_memory));
        memcpy(mapped_memory, pixels, image_size);
        vkUnmapMemory(s_context.device, staging_buffer_memory);

        // transition the image to transfer dst
        VkImageMemoryBarrier image_memory_barrier{};
        image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        image_memory_barrier.pNext = nullptr;
        image_memory_barrier.srcAccessMask = VK_ACCESS_NONE;
        image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        image_memory_barrier.image = out_texture.image;
        image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_memory_barrier.subresourceRange.baseMipLevel = 0;
        image_memory_barrier.subresourceRange.levelCount = num_mips;
        image_memory_barrier.subresourceRange.baseArrayLayer = 0;
        image_memory_barrier.subresourceRange.layerCount = 1;

        vkCmdPipelineBarrier(command_buffer,
                             VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0,
                             0, nullptr,
                             0, nullptr, 
                             1, &image_memory_barrier);

        // do a buffer copy from staging buffer to image
        VkImageSubresourceLayers subresource_layers{};
        subresource_layers.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresource_layers.mipLevel = 0;
        subresource_layers.baseArrayLayer = 0;
        subresource_layers.layerCount = 1;

        VkExtent3D image_extent{};
        image_extent.depth = 1;
        image_extent.height = tex_height;
        image_extent.width = tex_width;

        VkOffset3D image_offset{};
        image_offset.x = 0;
        image_offset.y = 0;
        image_offset.z = 0;

        VkBufferImageCopy buffer_to_image_copy{};
        buffer_to_image_copy.bufferOffset = 0;
        buffer_to_image_copy.bufferRowLength = tex_width;
        buffer_to_image_copy.bufferImageHeight = tex_height;
        buffer_to_image_copy.imageSubresource = subresource_layers;
        buffer_to_image_copy.imageOffset = image_offset;
        buffer_to_image_copy.imageExtent = image_extent;

        VkBufferImageCopy copy_regions[] = {
            buffer_to_image_copy
        };
        vkCmdCopyBufferToImage(command_buffer, staging_buffer, out_texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, ARRAY_LEN(copy_regions), copy_regions);
    }

    // generate mip maps for image and setup final image layout
    {
        for(u32 cur_mip_level = 0; cur_mip_level < num_mips; ++cur_mip_level)
        {
            u32 mip_level_read = cur_mip_level;
            u32 mip_level_write = cur_mip_level + 1;

            // transition the mip_level_read to transfer src
            {
                VkImageMemoryBarrier image_mip_read_memory_barrier{};
                image_mip_read_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                image_mip_read_memory_barrier.pNext = nullptr;
                image_mip_read_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                image_mip_read_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                image_mip_read_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                image_mip_read_memory_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                image_mip_read_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                image_mip_read_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                image_mip_read_memory_barrier.image = out_texture.image;
                image_mip_read_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                image_mip_read_memory_barrier.subresourceRange.baseMipLevel = mip_level_read;
                image_mip_read_memory_barrier.subresourceRange.levelCount = 1;
                image_mip_read_memory_barrier.subresourceRange.baseArrayLayer = 0;
                image_mip_read_memory_barrier.subresourceRange.layerCount = 1;

                VkImageMemoryBarrier image_mip_barriers[] = {
                    image_mip_read_memory_barrier,
                };
                vkCmdPipelineBarrier(command_buffer,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    0,
                    0, nullptr,
                    0, nullptr,
                    ARRAY_LEN(image_mip_barriers), image_mip_barriers);
            }

            // do actual copy from mip_level_read to mip_level_write
            if(mip_level_write < num_mips)
            {
                VkImageBlit blit_region{};
                blit_region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                blit_region.srcSubresource.mipLevel = mip_level_read;
                blit_region.srcSubresource.baseArrayLayer = 0;
                blit_region.srcSubresource.layerCount = 1;
                blit_region.srcOffsets[0].x = 0;
                blit_region.srcOffsets[0].y = 0;
                blit_region.srcOffsets[0].z = 0;
                blit_region.srcOffsets[1].x = max(tex_width >> mip_level_read, 1);
                blit_region.srcOffsets[1].y = max(tex_height >> mip_level_read, 1);
                blit_region.srcOffsets[1].z = 1;
                blit_region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                blit_region.dstSubresource.mipLevel = mip_level_write;
                blit_region.dstSubresource.baseArrayLayer = 0;
                blit_region.dstSubresource.layerCount = 1;
                blit_region.dstOffsets[0].x = 0;
                blit_region.dstOffsets[0].y = 0;
                blit_region.dstOffsets[0].z = 0;
                blit_region.dstOffsets[1].x = max(tex_width >> mip_level_write, 1);
                blit_region.dstOffsets[1].y = max(tex_height >> mip_level_write, 1);
                blit_region.dstOffsets[1].z = 1;

                vkCmdBlitImage(command_buffer,
                               out_texture.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               out_texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               1, &blit_region,
                               VK_FILTER_LINEAR);

            }
        }
                        
        // transition all mip levels to color attachment
        {
            VkImageMemoryBarrier image_to_color_attachment_memory_barrier{};
            image_to_color_attachment_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            image_to_color_attachment_memory_barrier.pNext = nullptr;
            image_to_color_attachment_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            image_to_color_attachment_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            image_to_color_attachment_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            image_to_color_attachment_memory_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            image_to_color_attachment_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            image_to_color_attachment_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            image_to_color_attachment_memory_barrier.image = out_texture.image;
            image_to_color_attachment_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            image_to_color_attachment_memory_barrier.subresourceRange.baseMipLevel = 0;
            image_to_color_attachment_memory_barrier.subresourceRange.levelCount = num_mips;
            image_to_color_attachment_memory_barrier.subresourceRange.baseArrayLayer = 0;
            image_to_color_attachment_memory_barrier.subresourceRange.layerCount = 1;

            VkImageMemoryBarrier image_mip_barriers[] = {
                image_to_color_attachment_memory_barrier,
            };
            vkCmdPipelineBarrier(command_buffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                0,
                0, nullptr,
                0, nullptr,
                ARRAY_LEN(image_mip_barriers), image_mip_barriers);
        }
    }

    // end and submit command buffer
    vkEndCommandBuffer(command_buffer);

    VkSubmitInfo command_submit_info{};
    command_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    command_submit_info.pNext = nullptr;
    command_submit_info.waitSemaphoreCount = 0;
    command_submit_info.pWaitSemaphores = nullptr;
    command_submit_info.pWaitDstStageMask = nullptr;
    VkCommandBuffer command_buffers[] = {
        command_buffer
    };
    command_submit_info.commandBufferCount = ARRAY_LEN(command_buffers);
    command_submit_info.pCommandBuffers = command_buffers;
    command_submit_info.signalSemaphoreCount = 0;
    command_submit_info.pSignalSemaphores = nullptr;

    SM_VULKAN_ASSERT(vkQueueSubmit(s_context.graphics_queue, 1, &command_submit_info, VK_NULL_HANDLE));

    // image view
    {
        VkComponentMapping components{};
        components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        VkImageSubresourceRange subresource_range{};
        subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresource_range.baseMipLevel = 0;
        subresource_range.levelCount = num_mips;
        subresource_range.baseArrayLayer = 0;
        subresource_range.layerCount = 1;

        VkImageViewCreateInfo image_view_create_info{};
        image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        image_view_create_info.pNext = nullptr;
        image_view_create_info.flags = 0;
        image_view_create_info.image = out_texture.image;
        image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        image_view_create_info.format = VK_FORMAT_R8G8B8A8_SRGB;
        image_view_create_info.components = components;
        image_view_create_info.subresourceRange = subresource_range;
        SM_VULKAN_ASSERT(vkCreateImageView(s_context.device, &image_view_create_info, nullptr, &out_texture.image_view));
    }
}

static void texture_release(texture_t& texture)
{
    vkDestroyImageView(s_context.device, texture.image_view, nullptr);
    vkDestroyImage(s_context.device, texture.image, nullptr);
    vkFreeMemory(s_context.device, texture.memory, nullptr);
}

static void buffer_init(buffer_t& out_buffer, size_t size, VkBufferUsageFlags usage_flags, VkMemoryPropertyFlags memory_flags)
{
    // buffer
    u32 queue_families[] = {
        (u32)s_context.queue_indices.graphics
    };

    VkBufferCreateInfo create_info{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = size,
        .usage = usage_flags,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = ARRAY_LEN(queue_families),
        .pQueueFamilyIndices = queue_families
    };
    SM_VULKAN_ASSERT(vkCreateBuffer(s_context.device, &create_info, nullptr, &out_buffer.buffer));

    // memory
    VkMemoryRequirements mem_requirements{};
    vkGetBufferMemoryRequirements(s_context.device, out_buffer.buffer, &mem_requirements);

    u32 memory_type_index = find_supported_memory_type_index(s_context, mem_requirements.memoryTypeBits, memory_flags);

    VkMemoryAllocateInfo alloc_info{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = mem_requirements.size,
        .memoryTypeIndex = memory_type_index
    };
    SM_VULKAN_ASSERT(vkAllocateMemory(s_context.device, &alloc_info, nullptr, &out_buffer.memory));

    // bind together
    SM_VULKAN_ASSERT(vkBindBufferMemory(s_context.device, out_buffer.buffer, out_buffer.memory, 0));
}

static void buffer_release(buffer_t& buffer)
{
    vkFreeMemory(s_context.device, buffer.memory, nullptr);
    vkDestroyBuffer(s_context.device, buffer.buffer, nullptr);
}

static void render_frames_init_render_targets()
{
    for(size_t i = 0; i < s_render_frames.cur_size; i++)
    {
        render_frame_t& frame = s_render_frames[i];

        texture_init(frame.forward_pass_draw_color_multisample_texture, 
                     s_context.default_color_format, 
                     VkExtent3D(s_context.swapchain.extent.width, s_context.swapchain.extent.height, 1), 
                     VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, 
                     VK_IMAGE_ASPECT_COLOR_BIT, 
                     s_context.max_msaa_samples, 
                     false);

        texture_init(frame.forward_pass_depth_multisample_texture, 
                     s_context.default_depth_format, 
                     VkExtent3D(s_context.swapchain.extent.width, s_context.swapchain.extent.height, 1), 
                     VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
                     VK_IMAGE_ASPECT_DEPTH_BIT, 
                     s_context.max_msaa_samples, 
                     false);

        texture_init(frame.forward_pass_color_resolve_texture, 
                     s_context.default_color_format, 
                     VkExtent3D(s_context.swapchain.extent.width, s_context.swapchain.extent.height, 1), 
                     VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT, 
                     VK_IMAGE_ASPECT_COLOR_BIT, 
                     VK_SAMPLE_COUNT_1_BIT, 
                     false);

        texture_init(frame.forward_pass_depth_resolve_texture, 
                     s_context.default_depth_format, 
                     VkExtent3D(s_context.swapchain.extent.width, s_context.swapchain.extent.height, 1), 
                     VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
                     VK_IMAGE_ASPECT_DEPTH_BIT, 
                     VK_SAMPLE_COUNT_1_BIT, 
                     false);

        texture_init(frame.post_processing_color_texture, 
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
            alloc_info.descriptorPool = s_frame_descriptor_pool;
            VkDescriptorSetLayout layouts[] = {
                s_frame_descriptor_set_layout
            };
            alloc_info.pSetLayouts = layouts;
            alloc_info.descriptorSetCount = ARRAY_LEN(layouts);
            vkAllocateDescriptorSets(s_context.device, &alloc_info, &frame.frame_descriptor_set);
        }

		// frame uniform buffer
        {
            buffer_init(frame.frame_descriptor_buffer, 
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
            buffer_init(frame.mesh_instance_render_data_staging_buffer,
                        sizeof(mesh_instance_render_data_t) * MAX_NUM_MESH_INSTANCES_PER_FRAME,
                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

            for(u32 mesh_instance_index = 0; mesh_instance_index < MAX_NUM_MESH_INSTANCES_PER_FRAME; ++mesh_instance_index)
            {
                buffer_init(frame.mesh_instance_render_data_buffers[mesh_instance_index], 
                            sizeof(mesh_instance_render_data_t), 
                            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            }
        }

		// post processing descriptor set
        {
            buffer_init(frame.post_processing_params_buffer, 
                        sizeof(post_processing_params_t), 
                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            VkDescriptorSetAllocateInfo alloc_info{};
            alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            alloc_info.descriptorPool = s_frame_descriptor_pool;
            VkDescriptorSetLayout set_layouts[] = {
                s_post_process_descriptor_set_layout
            };
            alloc_info.pSetLayouts = set_layouts;
            alloc_info.descriptorSetCount = ARRAY_LEN(set_layouts);
            SM_VULKAN_ASSERT(vkAllocateDescriptorSets(s_context.device, &alloc_info, &frame.post_processing_descriptor_set));
        }

        // infinite grid
        {
            // uniform buffer
            {
                buffer_init(frame.infinite_grid_data_buffer, 
                            sizeof(infinite_grid_data_t), 
                            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            }

            // descriptor set
            {
                VkDescriptorSetAllocateInfo alloc_info{};
                alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                alloc_info.descriptorPool = s_frame_descriptor_pool;

                VkDescriptorSetLayout descriptor_set_layouts[] = {
                    s_infinite_grid_descriptor_set_layout
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
        texture_release(frame.forward_pass_draw_color_multisample_texture);
        texture_release(frame.forward_pass_depth_multisample_texture);
        texture_release(frame.forward_pass_color_resolve_texture);
        texture_release(frame.forward_pass_depth_resolve_texture);
        texture_release(frame.post_processing_color_texture);
	}

	render_frames_init_render_targets();
}

static void pipelines_init()
{
    arena_t* temp_shader_arena = arena_init(KiB(50));

    // viking room pipeline
    {
        // shaders
        shader_t* vertex_shader = arena_alloc_struct(temp_shader_arena, shader_t);
        SM_ASSERT(shader_compiler_compile(temp_shader_arena, shader_type_t::VERTEX, "simple-material.vs.hlsl", "main", &vertex_shader));

        shader_t* pixel_shader = arena_alloc_struct(temp_shader_arena, shader_t);
        SM_ASSERT(shader_compiler_compile(temp_shader_arena, shader_type_t::PIXEL, "simple-material.ps.hlsl", "main", &pixel_shader));

        VkShaderModuleCreateInfo vertex_shader_module_create_info{};
        vertex_shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        vertex_shader_module_create_info.pNext = nullptr;
        vertex_shader_module_create_info.flags = 0;
        vertex_shader_module_create_info.codeSize = vertex_shader->bytecode.cur_size;
        vertex_shader_module_create_info.pCode = (u32*)vertex_shader->bytecode.data;

        VkShaderModule vertex_shader_module = VK_NULL_HANDLE;
        SM_VULKAN_ASSERT(vkCreateShaderModule(s_context.device, &vertex_shader_module_create_info, nullptr, &vertex_shader_module));

        VkShaderModuleCreateInfo pixel_shader_module_create_info{};
        pixel_shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        pixel_shader_module_create_info.pNext = nullptr;
        pixel_shader_module_create_info.flags = 0;
        pixel_shader_module_create_info.codeSize = pixel_shader->bytecode.cur_size;
        pixel_shader_module_create_info.pCode = (u32*)pixel_shader->bytecode.data;

        VkShaderModule pixel_shader_module = VK_NULL_HANDLE;
        SM_VULKAN_ASSERT(vkCreateShaderModule(s_context.device, &pixel_shader_module_create_info, nullptr, &pixel_shader_module));

        VkPipelineShaderStageCreateInfo vertex_stage{};
        vertex_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertex_stage.pNext = nullptr;
        vertex_stage.flags = 0;
        vertex_stage.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertex_stage.module = vertex_shader_module;
        vertex_stage.pName = vertex_shader->entry_name;
        vertex_stage.pSpecializationInfo = nullptr;

        VkPipelineShaderStageCreateInfo pixel_stage{};
        pixel_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        pixel_stage.pNext = nullptr;
        pixel_stage.flags = 0;
        pixel_stage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        pixel_stage.module = pixel_shader_module;
        pixel_stage.pName = pixel_shader->entry_name;
        pixel_stage.pSpecializationInfo = nullptr;

        VkPipelineShaderStageCreateInfo shader_stages[] = {
            vertex_stage,
            pixel_stage
        };

        // vertex input
        VkPipelineVertexInputStateCreateInfo vertex_input_state{};
        vertex_input_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertex_input_state.pNext = nullptr;
        vertex_input_state.flags = 0;

        VkVertexInputBindingDescription vertex_input_binding_description{};
        vertex_input_binding_description.binding = 0;
        vertex_input_binding_description.stride = sizeof(vertex_t);
        vertex_input_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        VkVertexInputBindingDescription vertex_input_binding_descriptions[] = {
            vertex_input_binding_description
        };

        vertex_input_state.vertexBindingDescriptionCount = ARRAY_LEN(vertex_input_binding_descriptions);
        vertex_input_state.pVertexBindingDescriptions = vertex_input_binding_descriptions;

        VkVertexInputAttributeDescription vertex_pos_attribute_description{};
        vertex_pos_attribute_description.location = 0;
        vertex_pos_attribute_description.binding = 0;
        vertex_pos_attribute_description.format = VK_FORMAT_R32G32B32_SFLOAT;
        vertex_pos_attribute_description.offset = offsetof(vertex_t, pos);

        VkVertexInputAttributeDescription vertex_uv_attribute_description{};
        vertex_uv_attribute_description.location = 1;
        vertex_uv_attribute_description.binding = 0;
        vertex_uv_attribute_description.format = VK_FORMAT_R32G32_SFLOAT;
        vertex_uv_attribute_description.offset = offsetof(vertex_t, uv);

        VkVertexInputAttributeDescription vertex_color_attribute_description{};
        vertex_color_attribute_description.location = 2;
        vertex_color_attribute_description.binding = 0;
        vertex_color_attribute_description.format = VK_FORMAT_R32G32B32_SFLOAT;
        vertex_color_attribute_description.offset = offsetof(vertex_t, color);

        VkVertexInputAttributeDescription vertex_normal_attribute_description{};
        vertex_normal_attribute_description.location = 3;
        vertex_normal_attribute_description.binding = 0;
        vertex_normal_attribute_description.format = VK_FORMAT_R32G32B32_SFLOAT;
        vertex_normal_attribute_description.offset = offsetof(vertex_t, normal);

        VkVertexInputAttributeDescription vertex_input_attributes_descriptions[] = {
            vertex_pos_attribute_description,
            vertex_uv_attribute_description,
            vertex_color_attribute_description,
            vertex_normal_attribute_description
        };

        vertex_input_state.vertexAttributeDescriptionCount = ARRAY_LEN(vertex_input_attributes_descriptions);
        vertex_input_state.pVertexAttributeDescriptions = vertex_input_attributes_descriptions;

        // input assembly
        VkPipelineInputAssemblyStateCreateInfo input_assembly{};
        input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        input_assembly.pNext = nullptr;
        input_assembly.flags = 0;
        input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        input_assembly.primitiveRestartEnable = VK_FALSE;

        // tessellation state
        VkPipelineTessellationStateCreateInfo tesselation_state{};
        tesselation_state.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
        tesselation_state.pNext = nullptr;
        tesselation_state.flags = 0;
        tesselation_state.patchControlPoints = 0;

        // viewport state
        VkPipelineViewportStateCreateInfo viewport_state{};
        viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_state.pNext = nullptr;
        viewport_state.flags = 0;

        VkViewport viewport{};
        viewport.x = 0;
        viewport.y = 0;
        viewport.width = (float)s_context.swapchain.extent.width;
        viewport.height = (float)s_context.swapchain.extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkViewport viewports[] = {
            viewport
        };

        viewport_state.viewportCount = ARRAY_LEN(viewports);
        viewport_state.pViewports = viewports;

        VkRect2D scissor{};
        scissor.offset.x = 0;
        scissor.offset.y = 0;
        scissor.extent.width = s_context.swapchain.extent.width;
        scissor.extent.height = s_context.swapchain.extent.height;

        VkRect2D scissors[] = {
            scissor
        };

        viewport_state.scissorCount = ARRAY_LEN(scissors);
        viewport_state.pScissors = scissors;

        // rasterization state
        VkPipelineRasterizationStateCreateInfo rasterization_state{};
        rasterization_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterization_state.pNext = nullptr;
        rasterization_state.flags = 0;
        rasterization_state.depthClampEnable = VK_FALSE;
        rasterization_state.rasterizerDiscardEnable = VK_FALSE;
        rasterization_state.polygonMode = VK_POLYGON_MODE_FILL;
        rasterization_state.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterization_state.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterization_state.depthBiasEnable = VK_FALSE;
        rasterization_state.depthBiasConstantFactor = 0.0f;
        rasterization_state.depthBiasClamp = 0.0f;
        rasterization_state.depthBiasSlopeFactor = 0.0f;
        rasterization_state.lineWidth = 1.0f;

        // multisample state
        VkPipelineMultisampleStateCreateInfo multisample_state{};
        multisample_state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisample_state.pNext = nullptr;
        multisample_state.flags = 0;
        multisample_state.rasterizationSamples = s_context.max_msaa_samples;
        multisample_state.sampleShadingEnable = VK_FALSE;
        multisample_state.minSampleShading = 0.0f;
        multisample_state.pSampleMask = nullptr;
        multisample_state.alphaToCoverageEnable = VK_FALSE;
        multisample_state.alphaToOneEnable = VK_FALSE;

        // depth stencil state
        VkPipelineDepthStencilStateCreateInfo depth_stencil_state{};
        depth_stencil_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depth_stencil_state.pNext = nullptr;
        depth_stencil_state.flags = 0;
        depth_stencil_state.depthTestEnable = VK_TRUE;
        depth_stencil_state.depthWriteEnable = VK_TRUE;
        depth_stencil_state.depthCompareOp = VK_COMPARE_OP_LESS;
        depth_stencil_state.depthBoundsTestEnable = VK_FALSE;
        depth_stencil_state.stencilTestEnable = VK_FALSE;

        depth_stencil_state.front.failOp = VK_STENCIL_OP_KEEP;
        depth_stencil_state.front.passOp = VK_STENCIL_OP_KEEP;
        depth_stencil_state.front.depthFailOp = VK_STENCIL_OP_KEEP;
        depth_stencil_state.front.compareOp = VK_COMPARE_OP_NEVER;
        depth_stencil_state.front.compareMask = 0;
        depth_stencil_state.front.writeMask = 0;
        depth_stencil_state.front.reference = 0;

        depth_stencil_state.back.failOp = VK_STENCIL_OP_KEEP;
        depth_stencil_state.back.passOp = VK_STENCIL_OP_KEEP;
        depth_stencil_state.back.depthFailOp = VK_STENCIL_OP_KEEP;
        depth_stencil_state.back.compareOp = VK_COMPARE_OP_NEVER;
        depth_stencil_state.back.compareMask = 0;
        depth_stencil_state.back.writeMask = 0;
        depth_stencil_state.back.reference = 0;

        depth_stencil_state.minDepthBounds = 0.0f;
        depth_stencil_state.maxDepthBounds = 1.0f;

        // color blend state
        VkPipelineColorBlendStateCreateInfo color_blend_state{};
        color_blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        color_blend_state.pNext = nullptr;
        color_blend_state.flags = 0;
        color_blend_state.logicOpEnable = VK_FALSE;
        color_blend_state.logicOp = VK_LOGIC_OP_CLEAR;

        VkPipelineColorBlendAttachmentState color_blend_attachment_state{};
        color_blend_attachment_state.blendEnable = VK_FALSE;
        color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        color_blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
        color_blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
        color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        color_blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        color_blend_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;
        color_blend_attachment_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                                      VK_COLOR_COMPONENT_G_BIT |
                                                      VK_COLOR_COMPONENT_B_BIT |
                                                      VK_COLOR_COMPONENT_A_BIT;

        VkPipelineColorBlendAttachmentState color_blend_attachment_states[] = {
            color_blend_attachment_state
        };

        color_blend_state.attachmentCount = ARRAY_LEN(color_blend_attachment_states);
        color_blend_state.pAttachments = color_blend_attachment_states;

        color_blend_state.blendConstants[0] = 0.0f;
        color_blend_state.blendConstants[1] = 0.0f;
        color_blend_state.blendConstants[2] = 0.0f;
        color_blend_state.blendConstants[3] = 0.0f;

        // dynamic state
        VkPipelineDynamicStateCreateInfo dynamic_state{};
        dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamic_state.pNext = nullptr;
        dynamic_state.flags = 0;
        dynamic_state.dynamicStateCount = 0;
        dynamic_state.pDynamicStates = nullptr;

        VkFormat color_formats[] = {
            s_context.default_color_format
        };

        VkPipelineRenderingCreateInfo pipeline_rendering_create_info{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
			.pNext = nullptr,
            .viewMask = 0,
			.colorAttachmentCount = ARRAY_LEN(color_formats),
			.pColorAttachmentFormats = color_formats,
			.depthAttachmentFormat = s_context.default_depth_format,
			.stencilAttachmentFormat = VK_FORMAT_UNDEFINED 
        };

        VkGraphicsPipelineCreateInfo pipeline_create_info{};
        pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_create_info.pNext = &pipeline_rendering_create_info;
        pipeline_create_info.flags = 0;
        pipeline_create_info.stageCount = ARRAY_LEN(shader_stages);
        pipeline_create_info.pStages = shader_stages;
        pipeline_create_info.pVertexInputState = &vertex_input_state;
        pipeline_create_info.pInputAssemblyState = &input_assembly;
        pipeline_create_info.pTessellationState = &tesselation_state;
        pipeline_create_info.pViewportState = &viewport_state;
        pipeline_create_info.pRasterizationState = &rasterization_state;
        pipeline_create_info.pMultisampleState = &multisample_state;
        pipeline_create_info.pDepthStencilState = &depth_stencil_state;
        pipeline_create_info.pColorBlendState = &color_blend_state;
        pipeline_create_info.pDynamicState = &dynamic_state;
        pipeline_create_info.layout = s_viking_room_material.pipeline_layouts[(u32)render_pass_t::FORWARD_PASS];
        pipeline_create_info.renderPass = VK_NULL_HANDLE;
        pipeline_create_info.subpass = 0;
        pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
        pipeline_create_info.basePipelineIndex = 0;

        VkGraphicsPipelineCreateInfo pipeline_create_infos[] = {
            pipeline_create_info
        };
        SM_VULKAN_ASSERT(
            vkCreateGraphicsPipelines(s_context.device, 
                                      VK_NULL_HANDLE, 
                                      ARRAY_LEN(pipeline_create_infos), 
                                      pipeline_create_infos, 
                                      nullptr, 
                                      &s_viking_room_material.pipelines[(u32)render_pass_t::FORWARD_PASS])
        );

        vkDestroyShaderModule(s_context.device, vertex_shader_module, nullptr);
        vkDestroyShaderModule(s_context.device, pixel_shader_module, nullptr);
    }

    // gizmo pipeline
    {
        // shaders
        shader_t* vertex_shader = arena_alloc_struct(temp_shader_arena, shader_t);
        SM_ASSERT(shader_compiler_compile(temp_shader_arena, shader_type_t::VERTEX, "gizmo.vs.hlsl", "main", &vertex_shader));

        shader_t* pixel_shader = arena_alloc_struct(temp_shader_arena, shader_t);
        SM_ASSERT(shader_compiler_compile(temp_shader_arena, shader_type_t::PIXEL, "gizmo.ps.hlsl", "main", &pixel_shader));

        VkShaderModuleCreateInfo vertex_shader_module_create_info{};
        vertex_shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        vertex_shader_module_create_info.pNext = nullptr;
        vertex_shader_module_create_info.flags = 0;
        vertex_shader_module_create_info.codeSize = vertex_shader->bytecode.cur_size;
        vertex_shader_module_create_info.pCode = (u32*)vertex_shader->bytecode.data;

        VkShaderModule vertex_shader_module = VK_NULL_HANDLE;
        SM_VULKAN_ASSERT(vkCreateShaderModule(s_context.device, &vertex_shader_module_create_info, nullptr, &vertex_shader_module));

        VkShaderModuleCreateInfo pixel_shader_module_create_info{};
        pixel_shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        pixel_shader_module_create_info.pNext = nullptr;
        pixel_shader_module_create_info.flags = 0;
        pixel_shader_module_create_info.codeSize = pixel_shader->bytecode.cur_size;
        pixel_shader_module_create_info.pCode = (u32*)pixel_shader->bytecode.data;

        VkShaderModule pixel_shader_module = VK_NULL_HANDLE;
        SM_VULKAN_ASSERT(vkCreateShaderModule(s_context.device, &pixel_shader_module_create_info, nullptr, &pixel_shader_module));

        VkPipelineShaderStageCreateInfo vertex_stage{};
        vertex_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertex_stage.pNext = nullptr;
        vertex_stage.flags = 0;
        vertex_stage.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertex_stage.module = vertex_shader_module;
        vertex_stage.pName = vertex_shader->entry_name;
        vertex_stage.pSpecializationInfo = nullptr;

        VkPipelineShaderStageCreateInfo pixel_stage{};
        pixel_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        pixel_stage.pNext = nullptr;
        pixel_stage.flags = 0;
        pixel_stage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        pixel_stage.module = pixel_shader_module;
        pixel_stage.pName = pixel_shader->entry_name;
        pixel_stage.pSpecializationInfo = nullptr;

        VkPipelineShaderStageCreateInfo shader_stages[] = {
            vertex_stage,
            pixel_stage
        };

        // vertex input
        VkPipelineVertexInputStateCreateInfo vertex_input_state{};
        vertex_input_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertex_input_state.pNext = nullptr;
        vertex_input_state.flags = 0;

        VkVertexInputBindingDescription vertex_input_binding_description{};
        vertex_input_binding_description.binding = 0;
        vertex_input_binding_description.stride = sizeof(vertex_t);
        vertex_input_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        VkVertexInputBindingDescription vertex_input_binding_descriptions[] = {
            vertex_input_binding_description
        };

        vertex_input_state.vertexBindingDescriptionCount = ARRAY_LEN(vertex_input_binding_descriptions);
        vertex_input_state.pVertexBindingDescriptions = vertex_input_binding_descriptions;

        VkVertexInputAttributeDescription vertex_pos_attribute_description{};
        vertex_pos_attribute_description.location = 0;
        vertex_pos_attribute_description.binding = 0;
        vertex_pos_attribute_description.format = VK_FORMAT_R32G32B32_SFLOAT;
        vertex_pos_attribute_description.offset = offsetof(vertex_t, pos);

        VkVertexInputAttributeDescription vertex_uv_attribute_description{};
        vertex_uv_attribute_description.location = 1;
        vertex_uv_attribute_description.binding = 0;
        vertex_uv_attribute_description.format = VK_FORMAT_R32G32_SFLOAT;
        vertex_uv_attribute_description.offset = offsetof(vertex_t, uv);

        VkVertexInputAttributeDescription vertex_color_attribute_description{};
        vertex_color_attribute_description.location = 2;
        vertex_color_attribute_description.binding = 0;
        vertex_color_attribute_description.format = VK_FORMAT_R32G32B32_SFLOAT;
        vertex_color_attribute_description.offset = offsetof(vertex_t, color);

        VkVertexInputAttributeDescription vertex_normal_attribute_description{};
        vertex_normal_attribute_description.location = 3;
        vertex_normal_attribute_description.binding = 0;
        vertex_normal_attribute_description.format = VK_FORMAT_R32G32B32_SFLOAT;
        vertex_normal_attribute_description.offset = offsetof(vertex_t, normal);

        VkVertexInputAttributeDescription vertex_input_attributes_descriptions[] = {
            vertex_pos_attribute_description,
            vertex_uv_attribute_description,
            vertex_color_attribute_description,
            vertex_normal_attribute_description
        };

        vertex_input_state.vertexAttributeDescriptionCount = ARRAY_LEN(vertex_input_attributes_descriptions);
        vertex_input_state.pVertexAttributeDescriptions = vertex_input_attributes_descriptions;

        // input assembly
        VkPipelineInputAssemblyStateCreateInfo input_assembly{};
        input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        input_assembly.pNext = nullptr;
        input_assembly.flags = 0;
        input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        input_assembly.primitiveRestartEnable = VK_FALSE;

        // tessellation state
        VkPipelineTessellationStateCreateInfo tesselation_state{};
        tesselation_state.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
        tesselation_state.pNext = nullptr;
        tesselation_state.flags = 0;
        tesselation_state.patchControlPoints = 0;

        // viewport state
        VkPipelineViewportStateCreateInfo viewport_state{};
        viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_state.pNext = nullptr;
        viewport_state.flags = 0;

        VkViewport viewport{};
        viewport.x = 0;
        viewport.y = 0;
        viewport.width = (float)s_context.swapchain.extent.width;
        viewport.height = (float)s_context.swapchain.extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkViewport viewports[] = {
            viewport
        };

        viewport_state.viewportCount = ARRAY_LEN(viewports);
        viewport_state.pViewports = viewports;

        VkRect2D scissor{};
        scissor.offset.x = 0;
        scissor.offset.y = 0;
        scissor.extent.width = s_context.swapchain.extent.width;
        scissor.extent.height = s_context.swapchain.extent.height;

        VkRect2D scissors[] = {
            scissor
        };

        viewport_state.scissorCount = ARRAY_LEN(scissors);
        viewport_state.pScissors = scissors;

        // rasterization state
        VkPipelineRasterizationStateCreateInfo rasterization_state{};
        rasterization_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterization_state.pNext = nullptr;
        rasterization_state.flags = 0;
        rasterization_state.depthClampEnable = VK_FALSE;
        rasterization_state.rasterizerDiscardEnable = VK_FALSE;
        rasterization_state.polygonMode = VK_POLYGON_MODE_FILL;
        rasterization_state.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterization_state.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterization_state.depthBiasEnable = VK_FALSE;
        rasterization_state.depthBiasConstantFactor = 0.0f;
        rasterization_state.depthBiasClamp = 0.0f;
        rasterization_state.depthBiasSlopeFactor = 0.0f;
        rasterization_state.lineWidth = 1.0f;

        // multisample state
        VkPipelineMultisampleStateCreateInfo multisample_state{};
        multisample_state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisample_state.pNext = nullptr;
        multisample_state.flags = 0;
        multisample_state.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisample_state.sampleShadingEnable = VK_FALSE;
        multisample_state.minSampleShading = 0.0f;
        multisample_state.pSampleMask = nullptr;
        multisample_state.alphaToCoverageEnable = VK_FALSE;
        multisample_state.alphaToOneEnable = VK_FALSE;

        // depth stencil state
        VkPipelineDepthStencilStateCreateInfo depth_stencil_state{};
        depth_stencil_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depth_stencil_state.pNext = nullptr;
        depth_stencil_state.flags = 0;
        depth_stencil_state.depthTestEnable = VK_TRUE;
        depth_stencil_state.depthWriteEnable = VK_TRUE;
        depth_stencil_state.depthCompareOp = VK_COMPARE_OP_LESS;
        depth_stencil_state.depthBoundsTestEnable = VK_FALSE;
        depth_stencil_state.stencilTestEnable = VK_FALSE;

        depth_stencil_state.front.failOp = VK_STENCIL_OP_KEEP;
        depth_stencil_state.front.passOp = VK_STENCIL_OP_KEEP;
        depth_stencil_state.front.depthFailOp = VK_STENCIL_OP_KEEP;
        depth_stencil_state.front.compareOp = VK_COMPARE_OP_NEVER;
        depth_stencil_state.front.compareMask = 0;
        depth_stencil_state.front.writeMask = 0;
        depth_stencil_state.front.reference = 0;

        depth_stencil_state.back.failOp = VK_STENCIL_OP_KEEP;
        depth_stencil_state.back.passOp = VK_STENCIL_OP_KEEP;
        depth_stencil_state.back.depthFailOp = VK_STENCIL_OP_KEEP;
        depth_stencil_state.back.compareOp = VK_COMPARE_OP_NEVER;
        depth_stencil_state.back.compareMask = 0;
        depth_stencil_state.back.writeMask = 0;
        depth_stencil_state.back.reference = 0;

        depth_stencil_state.minDepthBounds = 0.0f;
        depth_stencil_state.maxDepthBounds = 1.0f;

        // color blend state
        VkPipelineColorBlendStateCreateInfo color_blend_state{};
        color_blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        color_blend_state.pNext = nullptr;
        color_blend_state.flags = 0;
        color_blend_state.logicOpEnable = VK_FALSE;
        color_blend_state.logicOp = VK_LOGIC_OP_CLEAR;

        VkPipelineColorBlendAttachmentState color_blend_attachment_state{};
        color_blend_attachment_state.blendEnable = VK_FALSE;
        color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        color_blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
        color_blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
        color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        color_blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        color_blend_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;
        color_blend_attachment_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
            VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT;

        VkPipelineColorBlendAttachmentState color_blend_attachment_states[] = {
            color_blend_attachment_state
        };

        color_blend_state.attachmentCount = ARRAY_LEN(color_blend_attachment_states);
        color_blend_state.pAttachments = color_blend_attachment_states;

        color_blend_state.blendConstants[0] = 0.0f;
        color_blend_state.blendConstants[1] = 0.0f;
        color_blend_state.blendConstants[2] = 0.0f;
        color_blend_state.blendConstants[3] = 0.0f;

        // dynamic state
        VkPipelineDynamicStateCreateInfo dynamic_state{};
        dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamic_state.pNext = nullptr;
        dynamic_state.flags = 0;
        dynamic_state.dynamicStateCount = 0;
        dynamic_state.pDynamicStates = nullptr;

        VkFormat color_formats[] = {
            s_context.default_color_format,
        };

        VkFormat depth_format = s_context.default_depth_format;

        VkPipelineRenderingCreateInfo pipeline_rendering_create_info{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            .pNext = nullptr,
            .viewMask = 0,
            .colorAttachmentCount = ARRAY_LEN(color_formats),
            .pColorAttachmentFormats = color_formats,
            .depthAttachmentFormat = depth_format,
            .stencilAttachmentFormat = VK_FORMAT_UNDEFINED
        };

        VkGraphicsPipelineCreateInfo pipeline_create_info{};
        pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_create_info.pNext = &pipeline_rendering_create_info;
        pipeline_create_info.flags = 0;
        pipeline_create_info.stageCount = ARRAY_LEN(shader_stages);
        pipeline_create_info.pStages = shader_stages;
        pipeline_create_info.pVertexInputState = &vertex_input_state;
        pipeline_create_info.pInputAssemblyState = &input_assembly;
        pipeline_create_info.pTessellationState = &tesselation_state;
        pipeline_create_info.pViewportState = &viewport_state;
        pipeline_create_info.pRasterizationState = &rasterization_state;
        pipeline_create_info.pMultisampleState = &multisample_state;
        pipeline_create_info.pDepthStencilState = &depth_stencil_state;
        pipeline_create_info.pColorBlendState = &color_blend_state;
        pipeline_create_info.pDynamicState = &dynamic_state;
        pipeline_create_info.layout = s_gizmo_material.pipeline_layouts[(u32)render_pass_t::DEBUG_PASS];
        pipeline_create_info.renderPass = VK_NULL_HANDLE;
        pipeline_create_info.subpass = 0;
        pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
        pipeline_create_info.basePipelineIndex = 0;

        VkGraphicsPipelineCreateInfo pipeline_create_infos[] = {
            pipeline_create_info
        };
        SM_VULKAN_ASSERT(vkCreateGraphicsPipelines(s_context.device, VK_NULL_HANDLE, ARRAY_LEN(pipeline_create_infos), pipeline_create_infos, nullptr, &s_gizmo_material.pipelines[(u32)render_pass_t::DEBUG_PASS]));

        vkDestroyShaderModule(s_context.device, vertex_shader_module, nullptr);
        vkDestroyShaderModule(s_context.device, pixel_shader_module, nullptr);

        s_gizmo_material.descriptor_sets[(u32)render_pass_t::DEBUG_PASS] = s_empty_descriptor_set;
    }

    // post processing
    {
        shader_t* compute_shader = arena_alloc_struct(temp_shader_arena, shader_t);
        SM_ASSERT(shader_compiler_compile(temp_shader_arena, shader_type_t::COMPUTE, "post-processing.cs.hlsl", "main", &compute_shader));

        VkShaderModuleCreateInfo shader_module_create_info{
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .codeSize = compute_shader->bytecode.cur_size,
            .pCode = (u32*)compute_shader->bytecode.data
        };

        VkShaderModule shader_module = VK_NULL_HANDLE;
        SM_VULKAN_ASSERT(vkCreateShaderModule(s_context.device, &shader_module_create_info, nullptr, &shader_module));

        VkPipelineShaderStageCreateInfo post_process_shader_stage_create_info{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_COMPUTE_BIT,
            .module = shader_module,
            .pName = compute_shader->entry_name,
            .pSpecializationInfo = nullptr
        };

        VkComputePipelineCreateInfo post_process_create_info{
            .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = post_process_shader_stage_create_info,
            .layout = s_post_process_pipeline_layout,
            .basePipelineHandle = VK_NULL_HANDLE,
            .basePipelineIndex = 0 
        };
        VkComputePipelineCreateInfo pipeline_create_infos[] = {
            post_process_create_info
        };
        SM_VULKAN_ASSERT(vkCreateComputePipelines(s_context.device, VK_NULL_HANDLE, ARRAY_LEN(pipeline_create_infos), pipeline_create_infos, nullptr, &s_post_process_compute_pipeline));

        vkDestroyShaderModule(s_context.device, shader_module, nullptr);
    }

    arena_destroy(temp_shader_arena);
}

static void refresh_pipelines()
{
	vkDestroyPipeline(s_context.device, s_viking_room_material.pipelines[(u32)render_pass_t::FORWARD_PASS], nullptr);
	vkDestroyPipeline(s_context.device, s_gizmo_material.pipelines[(u32)render_pass_t::DEBUG_PASS], nullptr);
	pipelines_init();
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

static void renderer_mesh_init(sm::mesh_t& out_mesh, sm::mesh_data_t* mesh_data)
{
    // vertex buffer
    {
        size_t vertex_buffer_size = mesh_data_calc_vertex_buffer_size(mesh_data);
        buffer_init(out_mesh.vertex_buffer, 
                    vertex_buffer_size, 
                    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        upload_buffer_data(out_mesh.vertex_buffer.buffer, mesh_data->vertices.data, vertex_buffer_size);
    }

    // index buffer
    {
        size_t index_buffer_size = mesh_data_calc_index_buffer_size(mesh_data);
        buffer_init(out_mesh.index_buffer, 
                    index_buffer_size, 
                    VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        upload_buffer_data(out_mesh.index_buffer.buffer, mesh_data->indices.data, index_buffer_size);
    }

    out_mesh.num_indices = (u32)mesh_data->indices.cur_size;
}

static void gizmo_init(arena_t* arena)
{
    f32 gizmo_length = 0.75f;
    f32 gizmo_bar_thickness = 0.015f;
    f32 scale_box_thickness = 0.075f;

    // build translate tool mesh
    mesh_data_t* translate_mesh = mesh_init(arena);
    mesh_data_add_cylinder(translate_mesh, vec3_t::ZERO, vec3_t::WORLD_FORWARD, gizmo_length, gizmo_bar_thickness, 32, color_f32_t::RED);
    mesh_data_add_cone(translate_mesh, { .x = gizmo_length, .y = 0.0f, .z = 0.0f }, vec3_t::WORLD_FORWARD, 0.25f, 0.1f, 32, color_f32_t::RED);
    renderer_mesh_init(s_gizmo.translate_tool_gpu_mesh, translate_mesh);

    // build rotate tool mesh
    mesh_data_t* rotate_mesh = mesh_init(arena);
    mesh_data_add_torus(rotate_mesh, vec3_t::ZERO, vec3_t::WORLD_FORWARD, 0.65f, 0.025f, 32);
    renderer_mesh_init(s_gizmo.rotate_tool_gpu_mesh, rotate_mesh);

    // build scale tool mesh
    mesh_data_t* scale_mesh = mesh_init(arena);
    mesh_data_add_cylinder(scale_mesh, vec3_t::ZERO, vec3_t::WORLD_FORWARD, gizmo_length, gizmo_bar_thickness, 32, color_f32_t::RED);
    mesh_data_add_cube(scale_mesh, { .x = gizmo_length, .y = 0.0f, .z = 0.0f }, scale_box_thickness, 1, color_f32_t::RED);
    renderer_mesh_init(s_gizmo.scale_tool_gpu_mesh, scale_mesh);
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

	shader_compiler_init();
	mesh_data_init_primitives();
    mesh_instances_names_init();
    debug_draw_init();

    s_context = render_context_init(startup_arena, window);

    s_main_camera.world_pos = vec3_t{ .x = 3.0f, .y = 3.0f, .z = 3.0f };
    camera_look_at(s_main_camera, vec3_t::ZERO);

    gizmo_init(startup_arena);

	// descriptor pools
	{
		// global descriptor pool
		{
            VkDescriptorPoolSize pool_sizes[] = { 
				{ VK_DESCRIPTOR_TYPE_SAMPLER, 1 } 
			};

            VkDescriptorPoolCreateInfo create_info{};
            create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            create_info.poolSizeCount = (u32)ARRAY_LEN(pool_sizes);
            create_info.pPoolSizes = pool_sizes;
            create_info.maxSets = 2;
            SM_VULKAN_ASSERT(vkCreateDescriptorPool(s_context.device, &create_info, nullptr, &s_global_descriptor_pool));
		}

		// frame descriptor pool
		{
            // frame render data, infinite grid, post processing
            u32 num_uniform_buffers_per_frame = 2;
            u32 num_storage_images_per_frame = 2; // post processing input + output
            u32 num_sets_per_frame = 3;

            VkDescriptorPoolSize pool_sizes[] = { 
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_NUM_FRAMES_IN_FLIGHT * num_uniform_buffers_per_frame },
                { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, MAX_NUM_FRAMES_IN_FLIGHT * num_storage_images_per_frame } 
			};

            VkDescriptorPoolCreateInfo create_info{};
            create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            create_info.poolSizeCount = (u32)ARRAY_LEN(pool_sizes);
            create_info.pPoolSizes = pool_sizes;
            create_info.maxSets = MAX_NUM_FRAMES_IN_FLIGHT * num_sets_per_frame;
            SM_VULKAN_ASSERT(vkCreateDescriptorPool(s_context.device, &create_info, nullptr, &s_frame_descriptor_pool));
		}

		// material descriptor pool
		{
            VkDescriptorPoolSize pool_sizes[] = { 
				{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 20 }
			};

            VkDescriptorPoolCreateInfo create_info{};
            create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            create_info.poolSizeCount = (u32)ARRAY_LEN(pool_sizes);
            create_info.pPoolSizes = pool_sizes;
            create_info.maxSets = 20;
            SM_VULKAN_ASSERT(vkCreateDescriptorPool(s_context.device, &create_info, nullptr, &s_material_descriptor_pool));
		}

		// imgui descriptor pool
		{
            const i32 IMGUI_MAX_SETS = 1000;

            VkDescriptorPoolSize pool_sizes[] = { 
				{ VK_DESCRIPTOR_TYPE_SAMPLER, IMGUI_MAX_SETS },
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, IMGUI_MAX_SETS },
				{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, IMGUI_MAX_SETS },
				{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, IMGUI_MAX_SETS },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, IMGUI_MAX_SETS },
				{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, IMGUI_MAX_SETS },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, IMGUI_MAX_SETS },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, IMGUI_MAX_SETS },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, IMGUI_MAX_SETS },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, IMGUI_MAX_SETS },
				{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, IMGUI_MAX_SETS }
			};

            VkDescriptorPoolCreateInfo create_info{};
            create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            create_info.poolSizeCount = (u32)ARRAY_LEN(pool_sizes);
            create_info.pPoolSizes = pool_sizes;
            create_info.maxSets = IMGUI_MAX_SETS;
            SM_VULKAN_ASSERT(vkCreateDescriptorPool(s_context.device, &create_info, nullptr, &s_imgui_descriptor_pool));
		}
	}

	// descriptor set layouts
	{
        {
            VkDescriptorSetLayoutCreateInfo create_info{
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
				.bindingCount = 0,
				.pBindings = nullptr,
            };
            SM_VULKAN_ASSERT(vkCreateDescriptorSetLayout(s_context.device, &create_info, nullptr, &s_empty_descriptor_set_layout));
        }

		// global descriptor set layout
		{
			VkDescriptorSetLayoutBinding sampler_binding{};
			sampler_binding.binding = 0;
			sampler_binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
			sampler_binding.descriptorCount = 1;
			sampler_binding.stageFlags = VK_SHADER_STAGE_ALL;

            VkDescriptorSetLayoutBinding layout_bindings[] = {
				sampler_binding
            };

            VkDescriptorSetLayoutCreateInfo create_info{};
            create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            create_info.bindingCount = (u32)ARRAY_LEN(layout_bindings);
            create_info.pBindings = layout_bindings;
            SM_VULKAN_ASSERT(vkCreateDescriptorSetLayout(s_context.device, &create_info, nullptr, &s_global_descriptor_set_layout));
		}

		// frame descriptor set layout
		{
			VkDescriptorSetLayoutBinding uniform_binding{};
			uniform_binding.binding = 0;
			uniform_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			uniform_binding.descriptorCount = 1;
			uniform_binding.stageFlags = VK_SHADER_STAGE_ALL;

            VkDescriptorSetLayoutBinding layout_bindings[] = {
				uniform_binding
            };

            VkDescriptorSetLayoutCreateInfo create_info{};
            create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            create_info.bindingCount = (u32)ARRAY_LEN(layout_bindings);
            create_info.pBindings = layout_bindings;
            SM_VULKAN_ASSERT(vkCreateDescriptorSetLayout(s_context.device, &create_info, nullptr, &s_frame_descriptor_set_layout));
		}

        // infinite grid descriptor set layout
		{
			VkDescriptorSetLayoutBinding uniform_binding{};
			uniform_binding.binding = 0;
			uniform_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			uniform_binding.descriptorCount = 1;
			uniform_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

            VkDescriptorSetLayoutBinding layout_bindings[] = {
				uniform_binding 
            };

            VkDescriptorSetLayoutCreateInfo create_info{};
            create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            create_info.bindingCount = (u32)ARRAY_LEN(layout_bindings);
            create_info.pBindings = layout_bindings;
            SM_VULKAN_ASSERT(vkCreateDescriptorSetLayout(s_context.device, &create_info, nullptr, &s_infinite_grid_descriptor_set_layout));
		}

        // post processing descriptor set layout
		{
			VkDescriptorSetLayoutBinding src_image_binding{};
			src_image_binding.binding = 0;
			src_image_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			src_image_binding.descriptorCount = 1;
			src_image_binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

			VkDescriptorSetLayoutBinding dst_image_binding{};
			dst_image_binding.binding = 1;
			dst_image_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			dst_image_binding.descriptorCount = 1;
			dst_image_binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

			VkDescriptorSetLayoutBinding params_buffer_binding{};
			params_buffer_binding.binding = 2;
			params_buffer_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			params_buffer_binding.descriptorCount = 1;
			params_buffer_binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

            VkDescriptorSetLayoutBinding layout_bindings[] = {
				src_image_binding,
				dst_image_binding,
                params_buffer_binding
            };

            VkDescriptorSetLayoutCreateInfo create_info{};
            create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            create_info.bindingCount = (u32)ARRAY_LEN(layout_bindings);
            create_info.pBindings = layout_bindings;
            SM_VULKAN_ASSERT(vkCreateDescriptorSetLayout(s_context.device, &create_info, nullptr, &s_post_process_descriptor_set_layout));
		}

        // materials descriptor set layout
		{
			VkDescriptorSetLayoutBinding diffuse_texture_binding{};
			diffuse_texture_binding.binding = 0;
			diffuse_texture_binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			diffuse_texture_binding.descriptorCount = 1;
			diffuse_texture_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

            VkDescriptorSetLayoutBinding layout_bindings[] = {
				diffuse_texture_binding
            };

            VkDescriptorSetLayoutCreateInfo create_info{};
            create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            create_info.bindingCount = (u32)ARRAY_LEN(layout_bindings);
            create_info.pBindings = layout_bindings;
            SM_VULKAN_ASSERT(vkCreateDescriptorSetLayout(s_context.device, &create_info, nullptr, &s_material_descriptor_set_layout));
		}

        // mesh instance descriptor set layout
		{
			VkDescriptorSetLayoutBinding uniform_binding{};
			uniform_binding.binding = 0;
			uniform_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			uniform_binding.descriptorCount = 1;
			uniform_binding.stageFlags = VK_SHADER_STAGE_ALL;

            VkDescriptorSetLayoutBinding layout_bindings[] = {
				uniform_binding
            };

            VkDescriptorSetLayoutCreateInfo create_info{};
            create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            create_info.bindingCount = (u32)ARRAY_LEN(layout_bindings);
            create_info.pBindings = layout_bindings;
            SM_VULKAN_ASSERT(vkCreateDescriptorSetLayout(s_context.device, &create_info, nullptr, &s_mesh_instance_descriptor_set_layout));
		}
	}

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
        init_info.DescriptorPool = s_imgui_descriptor_pool;
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
		// global resources
		{
            // empty descriptor set
            {
                VkDescriptorSetLayout descriptor_set_layouts[] = {
                   s_empty_descriptor_set_layout 
                };

                VkDescriptorSetAllocateInfo alloc_info{
					.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
					.descriptorPool = s_global_descriptor_pool,
					.descriptorSetCount = ARRAY_LEN(descriptor_set_layouts),
					.pSetLayouts = descriptor_set_layouts
                };

                SM_VULKAN_ASSERT(vkAllocateDescriptorSets(s_context.device, &alloc_info, &s_empty_descriptor_set));
            }

			// global descriptor set
			{
                VkDescriptorSetAllocateInfo alloc_info{};
                alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                alloc_info.descriptorPool = s_global_descriptor_pool;

                VkDescriptorSetLayout descriptor_set_layouts[] = {
                   s_global_descriptor_set_layout 
                };
                alloc_info.descriptorSetCount = ARRAY_LEN(descriptor_set_layouts);
                alloc_info.pSetLayouts = descriptor_set_layouts;

                SM_VULKAN_ASSERT(vkAllocateDescriptorSets(s_context.device, &alloc_info, &s_global_descriptor_set));
			}

			// sampler
			{
                VkSamplerCreateInfo create_info{};
				create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
				create_info.flags = 0;
				create_info.magFilter = VK_FILTER_LINEAR;
				create_info.minFilter = VK_FILTER_LINEAR;
				create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
				create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
				create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
				create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
				create_info.mipLodBias = 0.0f;
				create_info.anisotropyEnable = VK_FALSE;
				create_info.maxAnisotropy = 0.0f;
				create_info.compareEnable = VK_FALSE;
				create_info.compareOp = VK_COMPARE_OP_NEVER;
				create_info.minLod = 0.0f;
				create_info.maxLod = VK_LOD_CLAMP_NONE;
				create_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
				create_info.unnormalizedCoordinates = VK_FALSE;
                SM_VULKAN_ASSERT(vkCreateSampler(s_context.device, &create_info, nullptr, &s_linear_sampler));
			}

			// write sampler to global descriptor set
			{
				VkWriteDescriptorSet sampler_write{};
				sampler_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				sampler_write.dstSet = s_global_descriptor_set;
				sampler_write.dstBinding = 0;
				sampler_write.dstArrayElement = 0;
				sampler_write.descriptorCount = 1;
				sampler_write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;

				VkDescriptorImageInfo image_info{};
				image_info.sampler = s_linear_sampler;
				image_info.imageView = VK_NULL_HANDLE;
				image_info.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				
				sampler_write.pImageInfo = &image_info;

				VkWriteDescriptorSet descriptor_set_writes[] = {
					sampler_write
				};
				
                vkUpdateDescriptorSets(s_context.device, ARRAY_LEN(descriptor_set_writes), descriptor_set_writes, 0, nullptr);
			}
		}

		// viking room
		{
            mesh_data_t* viking_room_obj = mesh_data_init_from_obj(startup_arena, "viking_room.obj");
            renderer_mesh_init(s_viking_room_mesh, viking_room_obj);

			// viking room diffuse texture
			{
                texture_init_from_file(s_viking_room_diffuse_texture, "viking-room.png", true);
			}

			// viking room descriptor set
			{
				// alloc
				VkDescriptorSetAllocateInfo alloc_info{};
				alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
				alloc_info.pNext = nullptr;
				alloc_info.descriptorPool = s_material_descriptor_pool;
				alloc_info.descriptorSetCount = 1;
				VkDescriptorSetLayout descriptor_set_layouts[] = {
					s_material_descriptor_set_layout
				};
				alloc_info.pSetLayouts = descriptor_set_layouts;
				SM_VULKAN_ASSERT(vkAllocateDescriptorSets(s_context.device, &alloc_info, &s_viking_room_material.descriptor_sets[(u32)render_pass_t::FORWARD_PASS]));

				// update
                VkDescriptorImageInfo descriptor_set_write_image_info{};
                descriptor_set_write_image_info.sampler = VK_NULL_HANDLE;
                descriptor_set_write_image_info.imageView = s_viking_room_diffuse_texture.image_view;
                descriptor_set_write_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                VkWriteDescriptorSet descriptor_set_write{};
                descriptor_set_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptor_set_write.pNext = nullptr;
                descriptor_set_write.dstSet = s_viking_room_material.descriptor_sets[(u32)render_pass_t::FORWARD_PASS];
                descriptor_set_write.dstBinding = 0;
                descriptor_set_write.dstArrayElement = 0;
                descriptor_set_write.descriptorCount = 1;
                descriptor_set_write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                descriptor_set_write.pImageInfo = &descriptor_set_write_image_info;
                descriptor_set_write.pBufferInfo = nullptr;
                descriptor_set_write.pTexelBufferView = nullptr;

                VkWriteDescriptorSet ds_writes[] = {
                    descriptor_set_write
                };

                vkUpdateDescriptorSets(s_context.device, ARRAY_LEN(ds_writes), ds_writes, 0, nullptr);
			}
		}

		// pipeline layouts
		{
			// viking room pipeline layout
			{
                VkDescriptorSetLayout pipeline_descriptor_set_layouts[] = {
                    s_global_descriptor_set_layout,
                    s_frame_descriptor_set_layout,
                    s_material_descriptor_set_layout,
                    s_mesh_instance_descriptor_set_layout
                };

                VkPipelineLayoutCreateInfo pipeline_layout_create_info{
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .setLayoutCount = ARRAY_LEN(pipeline_descriptor_set_layouts),
                    .pSetLayouts = pipeline_descriptor_set_layouts,
                    .pushConstantRangeCount = 0,
                    .pPushConstantRanges = nullptr
                };
                SM_VULKAN_ASSERT(vkCreatePipelineLayout(s_context.device, &pipeline_layout_create_info, nullptr, &s_viking_room_material.pipeline_layouts[(u32)render_pass_t::FORWARD_PASS]));
			}

            // gizmo pipeline layout
            {
                VkDescriptorSetLayout pipeline_descriptor_set_layouts[] = {
                    s_global_descriptor_set_layout,
                    s_frame_descriptor_set_layout,
                    s_empty_descriptor_set_layout,
                    s_mesh_instance_descriptor_set_layout
                };

                VkPushConstantRange push_constant_range{
                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .offset = 0,
                    .size = sizeof(gizmo_push_constants_t)
                };

                VkPushConstantRange push_constants[] = {
                    push_constant_range
                };

                VkPipelineLayoutCreateInfo pipeline_layout_create_info{
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .setLayoutCount = ARRAY_LEN(pipeline_descriptor_set_layouts),
                    .pSetLayouts = pipeline_descriptor_set_layouts,
                    .pushConstantRangeCount = ARRAY_LEN(push_constants),
                    .pPushConstantRanges = push_constants 
                };
                SM_VULKAN_ASSERT(vkCreatePipelineLayout(s_context.device, &pipeline_layout_create_info, nullptr, &s_gizmo_material.pipeline_layouts[(u32)render_pass_t::DEBUG_PASS]));
            }

            // infinite grid pipeline layout
			{
                VkDescriptorSetLayout pipeline_descriptor_set_layouts[] = {
					s_infinite_grid_descriptor_set_layout
                };

                VkPipelineLayoutCreateInfo pipeline_layout_create_info{
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .setLayoutCount = ARRAY_LEN(pipeline_descriptor_set_layouts),
                    .pSetLayouts = pipeline_descriptor_set_layouts,
                    .pushConstantRangeCount = 0,
                    .pPushConstantRanges = nullptr
                };
                SM_VULKAN_ASSERT(vkCreatePipelineLayout(s_context.device, &pipeline_layout_create_info, nullptr, &s_infinite_grid_forward_pass_pipeline_layout));
			}

            // post processing pipeline layout
			{
                VkDescriptorSetLayout pipeline_descriptor_set_layouts[] = {
                    s_post_process_descriptor_set_layout,
                    s_frame_descriptor_set_layout
                };

                VkPipelineLayoutCreateInfo pipeline_layout_create_info{
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .setLayoutCount = ARRAY_LEN(pipeline_descriptor_set_layouts),
                    .pSetLayouts = pipeline_descriptor_set_layouts,
                    .pushConstantRangeCount = 0,
                    .pPushConstantRanges = nullptr
                };
                SM_VULKAN_ASSERT(vkCreatePipelineLayout(s_context.device, &pipeline_layout_create_info, nullptr, &s_post_process_pipeline_layout));
			}
		}

        pipelines_init();
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
            mesh_instance_id_t added_mesh_instance = mesh_instances_add(&s_current_level->mesh_instances, &s_viking_room_mesh, &s_viking_room_material, push_constants, initial_transform, (u32)mesh_instance_flags_t::NONE);

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
    s.center = vec3_t::ZERO;
    s.radius = 2.0f;
    debug_draw_sphere(s, 1);
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

        upload_buffer_data(render_frame.frame_descriptor_buffer.buffer, &frame_data, sizeof(frame_render_data_t));

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
        mesh_t* mesh = render_frame.mesh_instances.meshes[i];
        material_t* material = render_frame.mesh_instances.materials[i];
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
            .pSetLayouts = &s_mesh_instance_descriptor_set_layout
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
            s_global_descriptor_set,
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
        upload_buffer_data(render_frame.post_processing_params_buffer.buffer, &post_process_params, sizeof(post_process_params));

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
                            s_post_process_pipeline_layout, 
                            0, 
                            ARRAY_LEN(post_processing_descriptor_sets), post_processing_descriptor_sets,
                            0, nullptr);
    vkCmdBindPipeline(render_frame.frame_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, s_post_process_compute_pipeline);
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
    if (s_selected_mesh_instance_id == INVALID_MESH_INSTANCE_ID)
    {
        return;
    }

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

static void gizmo_collect_mesh_instances(render_frame_t& render_frame)
{
    if (s_selected_mesh_instance_id == INVALID_MESH_INSTANCE_ID)
    {
        return;
    }

    mesh_instances_t mesh_instances;
    mesh_instances_init(render_frame.frame_arena, &mesh_instances, 3);

    transform_t selected_mesh_instance_transform = s_current_level->mesh_instances.transforms[s_selected_mesh_instance_id];

    mesh_t* gizmo_mesh_to_render = nullptr;
    switch (s_gizmo.mode)
    {
		case gizmo_mode_t::TRANSLATE: gizmo_mesh_to_render = &s_gizmo.translate_tool_gpu_mesh; break;
		case gizmo_mode_t::ROTATE: gizmo_mesh_to_render = &s_gizmo.rotate_tool_gpu_mesh; break;
		case gizmo_mode_t::SCALE: gizmo_mesh_to_render = &s_gizmo.scale_tool_gpu_mesh; break;
    }

    {
		transform_t gizmo_transform;
		set_translation(gizmo_transform.model, selected_mesh_instance_transform.model.tx, selected_mesh_instance_transform.model.ty, selected_mesh_instance_transform.model.tz);

		gizmo_push_constants_t* gizmo_push_constants = arena_alloc_struct(render_frame.frame_arena, gizmo_push_constants_t);
		gizmo_push_constants->color = to_vec3(color_f32_t::RED);

		push_constants_t push_constants;
		push_constants.data = gizmo_push_constants;
		push_constants.size = sizeof(gizmo_push_constants_t);

		mesh_instances_add(&mesh_instances, gizmo_mesh_to_render, &s_gizmo_material, push_constants, gizmo_transform, (u32)mesh_instance_flags_t::IS_DEBUG);
    }

    {
		transform_t gizmo_transform;
		rotate_z_degs(gizmo_transform.model, 90.0f);
		set_translation(gizmo_transform.model, selected_mesh_instance_transform.model.tx, selected_mesh_instance_transform.model.ty, selected_mesh_instance_transform.model.tz);

		gizmo_push_constants_t* gizmo_push_constants = arena_alloc_struct(render_frame.frame_arena, gizmo_push_constants_t);
		gizmo_push_constants->color = to_vec3(color_f32_t::GREEN);

		push_constants_t push_constants;
		push_constants.data = gizmo_push_constants;
		push_constants.size = sizeof(gizmo_push_constants_t);

		mesh_instances_add(&mesh_instances, gizmo_mesh_to_render, &s_gizmo_material, push_constants, gizmo_transform, (u32)mesh_instance_flags_t::IS_DEBUG);
    }

    {
		transform_t gizmo_transform;
		rotate_y_degs(gizmo_transform.model, -90.0f);
		set_translation(gizmo_transform.model, selected_mesh_instance_transform.model.tx, selected_mesh_instance_transform.model.ty, selected_mesh_instance_transform.model.tz);

		gizmo_push_constants_t* gizmo_push_constants = arena_alloc_struct(render_frame.frame_arena, gizmo_push_constants_t);
		gizmo_push_constants->color = to_vec3(color_f32_t::BLUE);

		push_constants_t push_constants;
		push_constants.data = gizmo_push_constants;
		push_constants.size = sizeof(gizmo_push_constants_t);

		mesh_instances_add(&mesh_instances, gizmo_mesh_to_render, &s_gizmo_material, push_constants, gizmo_transform, (u32)mesh_instance_flags_t::IS_DEBUG);
    }
    mesh_instances_append(&render_frame.mesh_instances, &mesh_instances);
}

static void debug_draw_system_collect_mesh_instances(render_frame_t& render_frame)
{
    gizmo_collect_mesh_instances(render_frame);
}

static void collect_mesh_instances(render_frame_t& render_frame)
{
    mesh_instances_init(render_frame.frame_arena, &render_frame.mesh_instances, MAX_NUM_MESH_INSTANCES_PER_FRAME);
    mesh_instances_append(&render_frame.mesh_instances, &s_current_level->mesh_instances);
    debug_draw_system_collect_mesh_instances(render_frame);
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
