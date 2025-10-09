#pragma once

#include "sm/core/array.h"
#include "sm/core/types.h"
#include "sm/render/vulkan/vk_include.h"

namespace sm
{
    struct arena_t;
    struct window_t;

    struct render_queue_indices_t
    {
        static const i32 INVALID_QUEUE_INDEX = -1;

        i32 graphics	            = INVALID_QUEUE_INDEX;
        i32 async_compute			= INVALID_QUEUE_INDEX;
        i32 presentation			= INVALID_QUEUE_INDEX;
        i32 transfer	            = INVALID_QUEUE_INDEX;
    };

    struct swapchain_info_t
    {
        VkSurfaceCapabilitiesKHR capabilities{};
        array_t<VkSurfaceFormatKHR> formats;
        array_t<VkPresentModeKHR> present_modes;
    };

    struct swapchain_t
    {
        VkSwapchainKHR handle = VK_NULL_HANDLE;
        array_t<VkImage> images;
        VkFormat format = VK_FORMAT_UNDEFINED;
        VkExtent2D extent = { 0 };
    };

    struct render_context_t
    {
        window_t* window = nullptr;
        VkInstance instance = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT debug_messenger = VK_NULL_HANDLE;
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        VkPhysicalDevice phys_device = VK_NULL_HANDLE;
        VkPhysicalDeviceProperties phys_device_props{};
        VkPhysicalDeviceMemoryProperties phys_device_mem_props{};
        VkDevice device = VK_NULL_HANDLE;
        render_queue_indices_t queue_indices;
        VkQueue graphics_queue = VK_NULL_HANDLE;
        VkQueue async_compute_queue = VK_NULL_HANDLE;
        VkQueue present_queue = VK_NULL_HANDLE;
        VkQueue transfer_queue = VK_NULL_HANDLE;
        VkCommandPool graphics_command_pool;
        swapchain_t swapchain;
        VkSampleCountFlagBits max_msaa_samples = VK_SAMPLE_COUNT_1_BIT;
        VkFormat default_color_format = VK_FORMAT_R8G8B8A8_UNORM;
        VkFormat default_depth_format = VK_FORMAT_UNDEFINED;
    };

    render_context_t render_context_init(arena_t* arena, window_t* window);
    void swapchain_init(render_context_t& context);
    u32 find_supported_memory_type_index(const render_context_t& context, u32 type_filter, VkMemoryPropertyFlags requested_flags);
};
