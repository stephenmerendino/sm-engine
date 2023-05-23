#pragma once

#include "engine/core/config.h"
#include "engine/core/types.h"
#include "engine/math/mat44.h"
#include "engine/render/vulkan/vulkan_include.h"
#include "engine/thirdparty/vulkan/vulkan_core.h"
#include <vector>

struct mesh_t;
struct window_t;

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

struct mvp_buffer_t
{
	mat44 model;
	mat44 view;
	mat44 projection;
};

struct transform_t
{
    mat44 model;
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
};

struct descriptor_set_t
{
    VkDescriptorSet descriptor_set;
};

struct renderable_mesh_t
{
    mesh_t* mesh = nullptr;
    buffer_t vertex_buffer;
    buffer_t index_buffer;
    transform_t transform;
    pipeline_t pipeline;
    descriptor_set_layout_t descriptor_set_layout;
    std::vector<descriptor_set_t> descriptor_sets;
};

struct frame_t
{
    u32 swapchain_image_index;
    semaphore_t swapchain_image_available_semaphore;
    semaphore_t render_finished_semaphore;
    fence_t frame_completed_fence;

    std::vector<VkCommandBuffer> command_buffers;

    descriptor_pool_t descriptor_pool;

    framebuffer_t main_draw_framebuffer;
    texture_t main_draw_color_target;
    texture_t main_draw_depth_target;
    texture_t main_draw_resolve_target;
};
