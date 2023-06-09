#pragma once

#include "engine/core/config.h"
#include "engine/core/guid.h"
#include "engine/core/types.h"
#include "engine/math/mat44.h"
#include "engine/render/mesh.h"
#include "engine/render/vulkan/vulkan_include.h"
#include "engine/thirdparty/vulkan/vulkan_core.h"
#include <vector>

struct mesh_t;
struct window_t;
struct camera_t;

typedef hash_id_t mesh_id_t;
typedef hash_id_t material_id_t;

struct instance_draw_data_t 
{
	mat44 mvp;
};

struct transform_t
{
    mat44 model;
};

struct semaphore_t
{   
    VkSemaphore handle;
};

struct fence_t
{
    VkFence handle;
};

struct instance_t
{
    VkInstance handle = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debug_messenger_handle = VK_NULL_HANDLE;
};

struct surface_t
{
    VkSurfaceKHR handle = VK_NULL_HANDLE;
};

struct queue_family_indices_t
{
    static const i32 INVALID = -1;

    i32 graphics_family = INVALID;
    i32 present_family = INVALID;
};

struct device_t
{
    VkPhysicalDevice phys_device_handle = VK_NULL_HANDLE;
    VkDevice device_handle = VK_NULL_HANDLE;
    VkQueue graphics_queue = VK_NULL_HANDLE;
    VkQueue present_queue = VK_NULL_HANDLE;
    VkSampleCountFlagBits max_num_msaa_samples = VK_SAMPLE_COUNT_1_BIT;
    queue_family_indices_t queue_families;
    VkPhysicalDeviceProperties phys_device_properties;
    VkFormat depth_format;
};

struct swapchain_details_t 
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> present_modes;
};

struct swapchain_t
{
    VkSwapchainKHR handle = VK_NULL_HANDLE; 
    VkFormat format = VK_FORMAT_UNDEFINED;
    VkExtent2D extent = { 0, 0 };
    u32 num_images = 0;
    std::vector<VkImage> images;
    std::vector<fence_t> image_in_flight_fences;
};

struct command_pool_t
{
    VkCommandPool handle = VK_NULL_HANDLE;
    VkQueueFlags queue_families;
    VkCommandPoolCreateFlags create_flags;
};

struct context_t
{
    window_t* window;
    instance_t instance;
    device_t device;
    surface_t surface;
    swapchain_t swapchain;
    command_pool_t graphics_command_pool;
};

enum class BufferType : u8
{
    kVertexBuffer,
    kIndexBuffer,
    kUniformBuffer,
    kStagingBuffer,
    kNumBufferTypes,
    kInvalidBuffer = 0xFF,
};

struct buffer_t
{
    VkBuffer handle = VK_NULL_HANDLE;   
    VkDeviceMemory memory = VK_NULL_HANDLE;
    size_t size;
    BufferType type;
};

struct texture_t
{
    VkImage handle = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView image_view;
    u32 num_mips;
};

struct sampler_t
{
    VkSampler handle = VK_NULL_HANDLE; 
    u32 max_mip_level = 1;
};

struct render_pass_attachments_t
{
	std::vector<VkAttachmentDescription> attachments;
};

struct subpass_t
{
	std::vector<VkAttachmentReference> color_attach_refs;
	std::vector<VkAttachmentReference> resolve_attach_refs;
	VkAttachmentReference depth_attach_ref;
	bool has_depth_attach;
};

enum class SubpassAttachmentRefType : u8
{
    COLOR,
    DEPTH,
    RESOLVE
};

struct subpasses_t
{
    std::vector<subpass_t> subpasses;
};

struct subpass_dependencies_t
{
    std::vector<VkSubpassDependency> dependencies;
};

struct render_pass_t
{
	VkRenderPass handle;
	render_pass_attachments_t attachments;
	subpasses_t subpasses;
	subpass_dependencies_t subpass_dependencies;
};

struct framebuffer_t
{
    VkFramebuffer handle;
};

struct pipeline_t
{
    VkPipeline pipeline_handle = VK_NULL_HANDLE;
    VkPipelineLayout layout_handle = VK_NULL_HANDLE;
};

struct pipeline_shader_stages_t
{
    std::vector<VkShaderModule> shaders;
    std::vector<VkPipelineShaderStageCreateInfo> shader_stage_infos;
};

struct pipeline_input_assembly_t
{
    VkPipelineInputAssemblyStateCreateInfo input_assembly_info = {};
};

struct pipeline_vertex_input_t
{
    VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
    std::vector<VkVertexInputBindingDescription> input_binding_descs;
    std::vector<VkVertexInputAttributeDescription> input_attr_descs;
};

struct pipeline_raster_state_t
{
    VkPipelineRasterizationStateCreateInfo raster_state = {};
};

struct pipeline_viewport_state_t
{
    VkPipelineViewportStateCreateInfo viewport_state = {};
    VkViewport viewport;
    VkRect2D scissor;
};

struct pipeline_multisample_state_t
{
    VkPipelineMultisampleStateCreateInfo multisample_state = {};
};

struct pipeline_depth_stencil_state_t
{
    VkPipelineDepthStencilStateCreateInfo depth_stencil_state = {};
};

struct pipeline_color_blend_state_t
{
    VkPipelineColorBlendStateCreateInfo color_blend_state = {};
    std::vector<VkPipelineColorBlendAttachmentState> color_blend_attachments;
};

struct pipeline_layout_t
{
    VkPipelineLayout pipeline_layout = VK_NULL_HANDLE; 
};

struct descriptor_set_layout_t
{
    VkDescriptorSetLayout handle = VK_NULL_HANDLE;
};

struct descriptor_pool_sizes_t
{
    std::vector<VkDescriptorPoolSize> pool_sizes;
};

struct descriptor_pool_t
{
    VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
    u32 max_num_sets = 0;
};

struct descriptor_set_t
{
    VkDescriptorSet descriptor_set = VK_NULL_HANDLE;
};

struct descriptor_set_write_info_t
{
    VkDescriptorType type;
    union
    {
        VkDescriptorBufferInfo buffer_write_info;
        VkDescriptorImageInfo image_write_info;
    };
};

struct descriptor_sets_writes_t
{
    std::vector<descriptor_set_write_info_t> descriptor_sets_write_infos;
    std::vector<VkWriteDescriptorSet> descriptor_sets_writes;
};

// TODO: need to support arrays in descriptors
struct descriptor_info_t
{
    VkDescriptorType type;
    u32 binding_index;
    u32 count;
    VkShaderStageFlagBits shader_stages;
};

struct descriptor_resource_t
{
    texture_t texture; // TODO(smerendino): make this a texture_id_t texture_id;
};

struct descriptor_set_layout_bindings_t
{
    std::vector<VkDescriptorSetLayoutBinding> bindings;
};

struct shader_info_t
{
    const char* shader_filepath;
    const char* shader_entry;
};

struct material_resource_t
{
    descriptor_info_t descriptor_info;      
    descriptor_resource_t descriptor_resource;
};

struct material_create_info_t
{
    shader_info_t vertex_shader_info;
    shader_info_t fragment_shader_info;
    std::vector<material_resource_t> resources;
};

typedef hash_id_t material_id_t;
struct material_t
{
    material_id_t id;
    pipeline_shader_stages_t shaders;
    std::vector<material_resource_t> resources;
    descriptor_set_layout_t descriptor_set_layout;
    descriptor_set_t descriptor_set;
};

struct mesh_render_data_t
{
    mesh_id_t mesh_id;
    u32 index_count;
    u32 vertex_count;
    VkPrimitiveTopology topology;
    buffer_t vertex_buffer;
    buffer_t index_buffer;
    pipeline_vertex_input_t pipeline_vertex_input; 
};

struct mesh_instance_render_data_t
{
    buffer_t data_buffer;
    descriptor_set_t descriptor_set;
    bool is_assigned = false;
};

typedef guid_t mesh_instance_id_t;
typedef hash_id_t name_id_t;
typedef u32 instance_draw_id_t;
struct mesh_instance_t 
{
    mesh_instance_id_t mesh_instance_id;
    name_id_t name_id;
    mesh_id_t mesh_id;
    material_id_t material_id;
    instance_draw_id_t instance_id;
    transform_t transform;
    pipeline_t pipeline; // TODO(smerendino): move this to resource manager
};

struct frame_render_data_t
{
    f32 time = 0.0f;
    f32 delta_time_seconds = 0.0f;
};

struct frame_t
{
    u32 swapchain_image_index;
    semaphore_t swapchain_image_available_semaphore;
    semaphore_t render_finished_semaphore;
    fence_t frame_completed_fence;
    fence_t swapchain_image_acquired_fence;
    semaphore_t swapchain_copied_semaphore;

    frame_render_data_t frame_render_data;
    buffer_t frame_render_data_buffer;
    descriptor_set_t frame_render_data_descriptor_set;

    descriptor_pool_t mesh_instance_render_data_descriptor_pool;
    std::vector<mesh_instance_render_data_t> mesh_instance_render_data;

    std::vector<VkCommandBuffer> command_buffers;
    VkCommandBuffer copy_to_backbuffer_command_buffer;

    framebuffer_t main_draw_framebuffer;
    texture_t main_draw_color_target;
    texture_t main_draw_depth_target;
    texture_t main_draw_resolve_target;

    framebuffer_t imgui_framebuffer;
};

struct renderer_globals_t
{
    std::vector<frame_t> in_flight_frames;
    std::vector<VkFence> swapchain_images_in_flight;
    u32 cur_frame = 0;

    descriptor_set_layout_t mesh_instance_render_data_ds_layout;

    // global descriptors
    sampler_t linear_sampler_2d;
    descriptor_set_layout_t global_data_ds_layout;
    descriptor_pool_t global_data_dp;
    descriptor_set_t global_ds;

    // frame data descriptor setup
    descriptor_set_layout_t frame_render_data_descriptor_set_layout;
    descriptor_pool_t frame_render_data_descriptor_pool;

    // material data descriptor pool and empty set
    descriptor_pool_t material_data_dp;
    descriptor_set_t empty_descriptor_set;

    // render passes
    render_pass_t main_draw_render_pass;

    // imgui
    render_pass_t imgui_render_pass;
    descriptor_pool_t imgui_dp;
    
    camera_t* main_camera = nullptr;

    bool debug_render = false;
};

