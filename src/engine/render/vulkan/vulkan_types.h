#pragma once

#include "engine/core/Types.h"
#include "engine/math/mat44.h"
#include "engine/render/vulkan/vulkan_include.h"
#include <vector>

struct mesh_t;
struct window_t;

struct semaphore_t
{   
    VkSemaphore m_handle;
};

struct fence_t
{
    VkFence m_handle;
};

struct instance_t
{
    VkInstance m_handle = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_debug_messenger_handle = VK_NULL_HANDLE;
};

struct surface_t
{
    VkSurfaceKHR m_handle = VK_NULL_HANDLE;
};

struct queue_family_indices_t
{
    static const i32 INVALID = -1;

    i32 m_graphics_family = INVALID;
    i32 m_present_family = INVALID;
};

struct device_t
{
    VkPhysicalDevice m_phys_device_handle = VK_NULL_HANDLE;
    VkDevice m_device_handle = VK_NULL_HANDLE;
    VkQueue m_graphics_queue = VK_NULL_HANDLE;
    VkQueue m_present_queue = VK_NULL_HANDLE;
    VkSampleCountFlagBits m_max_num_msaa_samples = VK_SAMPLE_COUNT_1_BIT;
    queue_family_indices_t m_queue_families;
    VkPhysicalDeviceProperties m_phys_device_properties;
};

struct swapchain_details_t 
{
    VkSurfaceCapabilitiesKHR m_capabilities;
    std::vector<VkSurfaceFormatKHR> m_formats;
    std::vector<VkPresentModeKHR> m_present_modes;
};

struct swapchain_t
{
    VkSwapchainKHR m_handle = VK_NULL_HANDLE; 
    VkFormat m_format = VK_FORMAT_UNDEFINED;
    VkExtent2D m_extent = { 0, 0 };
    u32 m_num_images = 0;
    std::vector<VkImage> m_images;
    std::vector<VkImageView> m_image_views;
    std::vector<fence_t> m_image_in_flight_fences;
};

struct command_pool_t
{
    VkCommandPool m_handle = VK_NULL_HANDLE;
    VkQueueFlags m_queue_families;
    VkCommandPoolCreateFlags m_create_flags;
};

struct context_t
{
    window_t* m_window;
    instance_t m_instance;
    device_t m_device;
    surface_t m_surface;
    swapchain_t m_swapchain;
    command_pool_t m_graphics_command_pool;
};

struct frame_t
{
    u32 m_swap_chain_image_index;
    semaphore_t m_image_available_semaphore;
    semaphore_t m_render_finished_semaphore;
    fence_t m_frame_completed_fence;
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
    VkBuffer m_handle = VK_NULL_HANDLE;   
    VkDeviceMemory m_memory = VK_NULL_HANDLE;
    size_t m_size;
    BufferType m_type;
};

struct renderable_mesh_t
{
    mesh_t* m_mesh = nullptr;
    buffer_t m_vertex_buffer;
    buffer_t m_index_buffer;
};

struct texture_t
{
    VkImage m_handle = VK_NULL_HANDLE;
    VkDeviceMemory m_memory = VK_NULL_HANDLE;
    VkImageView m_image_view;
    u32 m_num_mips;
};

struct sampler_t
{
    VkSampler m_handle = VK_NULL_HANDLE; 
    u32 m_max_mip_level = 1;
};

struct subpass_t
{
	std::vector<VkAttachmentReference> m_color_attach_refs;
	std::vector<VkAttachmentReference> m_resolve_attach_refs;
	VkAttachmentReference m_depth_attach_ref;
	bool m_has_depth_attach;
};

struct render_pass_t
{
	VkRenderPass m_handle;
	std::vector<VkSubpassDescription> m_subpasses;
	std::vector<VkSubpassDependency> m_subpass_dependencies;
	std::vector<VkAttachmentDescription> m_attachments;
    
};

struct framebuffer_t
{
    VkFramebuffer m_handle;
};

struct pipeline_t
{
    VkPipeline m_pipeline_handle = VK_NULL_HANDLE;
    VkPipelineLayout m_pipeline_layout_handle = VK_NULL_HANDLE;
    std::vector<VkPipelineColorBlendAttachmentState> m_color_blend_attachments;
    VkViewport m_viewport;
    VkRect2D m_scissor;
    std::vector<VkShaderModule> m_shaders;
    std::vector<VkPipelineShaderStageCreateInfo> m_shader_stage_infos;
    VkPipelineInputAssemblyStateCreateInfo m_input_assembly_info;
    VkPipelineRasterizationStateCreateInfo m_raster_info;
    VkPipelineMultisampleStateCreateInfo m_multisample_info;
    VkPipelineDepthStencilStateCreateInfo m_depth_stencil_info;
    VkPipelineColorBlendStateCreateInfo m_color_blend_info;
    VkPipelineLayoutCreateInfo m_pipeline_layout_info;
};

struct mvp_buffer_t
{
	mat44 m_model;
	mat44 m_view;
	mat44 m_projection;
};
