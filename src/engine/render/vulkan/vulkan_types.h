#pragma once

#include "engine/core/Types.h"
#include "engine/math/mat44.h"
#include "engine/render/vulkan/vulkan_include.h"
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
    //std::vector<VkImageView> image_views;
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

struct frame_t
{
    u32 swapchain_image_index;
    semaphore_t swapchain_image_available_semaphore;
    semaphore_t render_finished_semaphore;
    fence_t frame_completed_fence;
    std::vector<VkCommandBuffer> command_buffers;
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
    VkPipelineLayout pipeline_layout_handle = VK_NULL_HANDLE;
    std::vector<VkPipelineColorBlendAttachmentState> color_blend_attachments;
    VkViewport viewport;
    VkRect2D scissor;
    std::vector<VkShaderModule> shaders;
    std::vector<VkPipelineShaderStageCreateInfo> shader_stage_infos;
    VkPipelineInputAssemblyStateCreateInfo input_assembly_info;
    VkPipelineRasterizationStateCreateInfo raster_info;
    VkPipelineMultisampleStateCreateInfo multisample_info;
    VkPipelineDepthStencilStateCreateInfo depth_stencil_info;
    VkPipelineColorBlendStateCreateInfo color_blend_info;
    VkPipelineLayoutCreateInfo pipeline_layout_info;
};

struct mvp_buffer_t
{
	mat44 model;
	mat44 view;
	mat44 projection;
};

struct renderable_mesh_t
{
    mesh_t* mesh = nullptr;
    pipeline_t* pipeline = nullptr;
    buffer_t vertex_buffer;
    buffer_t index_buffer;
};
