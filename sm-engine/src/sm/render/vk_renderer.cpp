#include "sm/render/vk_renderer.h"
#include "sm/config.h"
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
#include "sm/render/vk_include.h"

#include "third_party/imgui/imgui.h"
#include "third_party/imgui/backends/imgui_impl_win32.h"
#include "third_party/imgui/backends/imgui_impl_vulkan.h"

#pragma warning(push)
#pragma warning(disable:4244)
#define STB_IMAGE_IMPLEMENTATION
#include "third_party/stb/stb_image.h"
#pragma warning(pop)

using namespace sm;

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

struct vk_queue_indices_t
{
	static const i32 INVALID_QUEUE_INDEX = -1;

	i32 graphics_and_compute	= INVALID_QUEUE_INDEX;
	i32 async_compute			= INVALID_QUEUE_INDEX;
	i32 presentation			= INVALID_QUEUE_INDEX;
};

struct vk_swapchain_info_t
{
	VkSurfaceCapabilitiesKHR capabilities = { 0 };
	array_t<VkSurfaceFormatKHR> formats;
	array_t<VkPresentModeKHR> present_modes;
};

struct render_frame_t 
{
	// swapchain
	u32	swapchain_image_index = UINT32_MAX;
	VkSemaphore swapchain_image_is_ready_semaphore;

	// frame level resources
	VkFence frame_completed_fence;
	VkSemaphore frame_completed_semaphore;
	VkDescriptorSet frame_descriptor_set;
	VkBuffer frame_descriptor_buffer;
	VkDeviceMemory frame_descriptor_buffer_memory;
	VkCommandBuffer frame_command_buffer;

	// main draw resources
	VkImage main_draw_color_multisample_image;
	VkImageView main_draw_color_multisample_image_view;
	VkDeviceMemory main_draw_color_multisample_device_memory;
	u32 main_draw_color_multisample_num_mips;

	VkImage main_draw_depth_multisample_image;
	VkImageView main_draw_depth_multisample_image_view;
	VkDeviceMemory main_draw_depth_multisample_device_memory;
	u32 main_draw_depth_multisample_num_mips;

	VkImage main_draw_color_resolve_image;
	VkImageView main_draw_color_resolve_image_view;
	VkDeviceMemory main_draw_color_resolve_device_memory;
	u32 main_draw_color_resolve_num_mips;

	VkFramebuffer main_draw_framebuffer;

	// mesh instance descriptors
	VkDescriptorPool mesh_instance_descriptor_pool;

	// post processing
	VkImage post_processing_color_image;
	VkImageView post_processing_color_image_view;
	VkDeviceMemory post_processing_color_device_memory;
	u32 post_processing_color_num_mips;
	VkDescriptorSet	post_processing_descriptor_set;

	// imgui 
	VkFramebuffer imgui_framebuffer;

	// infinite grid
    VkBuffer infinite_grid_data_buffer;
    VkDeviceMemory infinite_grid_buffer_device_memory;
    VkDeviceSize infinite_grid_buffer_device_size;
	VkDescriptorSet infinite_grid_descriptor_set;
};

window_t* s_window = nullptr;
static bool s_close_window = false;

VkInstance s_instance = VK_NULL_HANDLE;
VkDebugUtilsMessengerEXT s_debug_messenger = VK_NULL_HANDLE;
VkSurfaceKHR s_surface = VK_NULL_HANDLE;
VkPhysicalDevice s_phys_device = VK_NULL_HANDLE;
VkPhysicalDeviceProperties s_phys_device_props{};
VkPhysicalDeviceMemoryProperties s_phys_device_mem_props{};
VkDevice s_device = VK_NULL_HANDLE;

vk_queue_indices_t s_queue_indices;
VkQueue s_graphics_queue = VK_NULL_HANDLE;
VkQueue s_async_compute_queue = VK_NULL_HANDLE;
VkQueue s_present_queue = VK_NULL_HANDLE;

VkCommandPool s_graphics_command_pool;

VkSampleCountFlagBits s_max_msaa_samples = VK_SAMPLE_COUNT_1_BIT;
VkFormat s_main_color_format = VK_FORMAT_R8G8B8A8_UNORM;
VkFormat s_depth_format = VK_FORMAT_UNDEFINED;

VkSwapchainKHR s_swapchain = VK_NULL_HANDLE;
array_t<VkImage> s_swapchain_images;
VkFormat s_swapchain_format = VK_FORMAT_UNDEFINED;
VkExtent2D s_swapchain_extent{};

VkDescriptorPool s_global_descriptor_pool = VK_NULL_HANDLE;
VkDescriptorPool s_frame_descriptor_pool = VK_NULL_HANDLE;
VkDescriptorPool s_material_descriptor_pool = VK_NULL_HANDLE;
VkDescriptorPool s_imgui_descriptor_pool = VK_NULL_HANDLE;
VkDescriptorSetLayout s_global_descriptor_set_layout = VK_NULL_HANDLE;
VkDescriptorSetLayout s_frame_descriptor_set_layout = VK_NULL_HANDLE;
VkDescriptorSetLayout s_infinite_grid_descriptor_set_layout = VK_NULL_HANDLE;
VkDescriptorSetLayout s_post_process_descriptor_set_layout = VK_NULL_HANDLE;
VkDescriptorSetLayout s_material_descriptor_set_layout = VK_NULL_HANDLE;
VkDescriptorSetLayout s_mesh_instance_descriptor_set_layout = VK_NULL_HANDLE;

VkDescriptorSet s_global_descriptor_set = VK_NULL_HANDLE;
VkSampler s_linear_sampler = VK_NULL_HANDLE;

VkRenderPass s_main_draw_render_pass;
VkRenderPass s_imgui_render_pass;

array_t<render_frame_t> s_render_frames;

// viking room mesh/material resources
mesh_t* s_viking_room_mesh = nullptr;
VkBuffer s_viking_room_vertex_buffer = VK_NULL_HANDLE;
VkDeviceMemory s_viking_room_vertex_buffer_memory = VK_NULL_HANDLE;
VkBuffer s_viking_room_index_buffer = VK_NULL_HANDLE;
VkDeviceMemory s_viking_room_index_buffer_memory = VK_NULL_HANDLE;
VkImage s_viking_room_diffuse_texture_image = VK_NULL_HANDLE;
VkDeviceMemory s_viking_room_diffuse_texture_memory = VK_NULL_HANDLE;
VkImageView s_viking_room_diffuse_texture_image_view = VK_NULL_HANDLE;
VkDescriptorSet s_viking_room_material_descriptor_set = VK_NULL_HANDLE;
VkBuffer s_viking_room_mesh_instance_buffer = VK_NULL_HANDLE;
VkDeviceMemory s_viking_room_mesh_instance_buffer_memory = VK_NULL_HANDLE;
VkPipelineLayout s_viking_room_main_draw_pipeline_layout = VK_NULL_HANDLE;
VkPipeline s_viking_room_main_draw_pipeline = VK_NULL_HANDLE;

VkPipelineLayout s_infinite_grid_main_draw_pipeline_layout = VK_NULL_HANDLE;
VkPipelineLayout s_post_process_pipeline_layout = VK_NULL_HANDLE;

camera_t s_main_camera;
f32 s_elapsed_time_seconds = 0.0f;
f32 s_delta_time_seconds = 0.0f;
u64 s_cur_frame_number = 0;
u8 s_cur_render_frame = 0;

static bool format_has_stencil(VkFormat format)
{
    return (format == VK_FORMAT_D32_SFLOAT_S8_UINT) || 
           (format == VK_FORMAT_D24_UNORM_S8_UINT);
}

static VkFormat find_supported_format(VkPhysicalDevice phys_device, VkFormat* candidates, u32 num_candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	for(u32 i = 0; i < num_candidates; i++)
	{
		const VkFormat& format = candidates[i];

		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(phys_device, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
		{
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
		{
			return format;
		}
	}

	return VK_FORMAT_UNDEFINED;
}

static VkFormat find_supported_format(VkPhysicalDevice phys_device, const array_t<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	return find_supported_format(phys_device, candidates.data, (u32)candidates.cur_size, tiling, features);
}

static VkFormat find_supported_depth_format(VkPhysicalDevice phys_device)
{
	VkFormat possibles[] = { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
	VkFormat depthFormat = find_supported_format(phys_device, possibles, ARRAY_LEN(possibles), VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	SM_ASSERT(depthFormat != VK_FORMAT_UNDEFINED);
	return depthFormat;
}

static u32 find_supported_memory_type(VkPhysicalDeviceMemoryProperties device_mem_props, u32 type_filter, VkMemoryPropertyFlags requested_flags)
{
	u32 found_mem_type = UINT_MAX;
	for (u32 i = 0; i < device_mem_props.memoryTypeCount; i++)
	{
		if (type_filter & (1 << i) && (device_mem_props.memoryTypes[i].propertyFlags & requested_flags) == requested_flags)
		{
			found_mem_type = i;
			break;
		}
	}

	SM_ASSERT(found_mem_type != UINT_MAX);
	return found_mem_type;
}

static vk_queue_indices_t find_queue_indices(arena_t* arena, VkPhysicalDevice device, VkSurfaceKHR surface)
{
	u32 queue_familiy_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_familiy_count, nullptr);

	array_t<VkQueueFamilyProperties> queue_family_props = array_init_sized<VkQueueFamilyProperties>(arena, queue_familiy_count);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_familiy_count, queue_family_props.data);

	vk_queue_indices_t indices;

	for (int i = 0; i < (int)queue_family_props.cur_size; i++)
	{
		const VkQueueFamilyProperties& props = queue_family_props[i];
		if (indices.graphics_and_compute == vk_queue_indices_t::INVALID_QUEUE_INDEX && 
			(props.queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) == (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT))
		{
			indices.graphics_and_compute = i;
		}

		if (indices.async_compute == vk_queue_indices_t::INVALID_QUEUE_INDEX && 
			(props.queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) == VK_QUEUE_COMPUTE_BIT)
		{
			indices.async_compute = i;
		}

		VkBool32 can_present = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &can_present);
		if (indices.presentation == vk_queue_indices_t::INVALID_QUEUE_INDEX && can_present)
		{
			indices.presentation = i;
		}

		// check if no more families to find
		if (indices.graphics_and_compute != vk_queue_indices_t::INVALID_QUEUE_INDEX && 
			indices.async_compute != vk_queue_indices_t::INVALID_QUEUE_INDEX &&
			indices.presentation != vk_queue_indices_t::INVALID_QUEUE_INDEX)
		{
			break;
		}
	}

	return indices;
}

static bool has_required_queues(const vk_queue_indices_t& queue_indices)
{
	return queue_indices.graphics_and_compute != vk_queue_indices_t::INVALID_QUEUE_INDEX && 
		   queue_indices.presentation != vk_queue_indices_t::INVALID_QUEUE_INDEX;
}

static vk_swapchain_info_t query_swapchain(arena_t* arena, VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
	vk_swapchain_info_t details;

	// surface capabilities
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &details.capabilities);

	// surface formats
	u32 num_surface_formats = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &num_surface_formats, nullptr);

	if (num_surface_formats != 0)
	{
		details.formats = array_init_sized<VkSurfaceFormatKHR>(arena, num_surface_formats);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &num_surface_formats, details.formats.data);
	}

	// present modes
	u32 num_present_modes = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &num_present_modes, nullptr);

	if (num_present_modes != 0)
	{
		details.present_modes = array_init_sized<VkPresentModeKHR>(arena, num_present_modes);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &num_present_modes, details.present_modes.data);
	}

	return details;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_func(VkDebugUtilsMessageSeverityFlagBitsEXT      msg_severity,
													VkDebugUtilsMessageTypeFlagsEXT             msg_type,
													const VkDebugUtilsMessengerCallbackDataEXT* cb_data,
													void*										user_data)
{
	UNUSED(msg_type);
	UNUSED(user_data);

	// filter out verbose and info messages unless we explicitly want them
	if (!VULKAN_VERBOSE)
	{
		if (msg_severity & (VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT))
		{
			return VK_FALSE;
		}
	}

	debug_printf("[vk");

	switch (msg_type)
	{
		case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:		debug_printf("-general");		break;
		case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:	debug_printf("-performance");	break;
		case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:	debug_printf("-validation");	break;
	}

	switch (msg_severity)
	{
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:	debug_printf("-verbose]");	break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:		debug_printf("-info]");		break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:	debug_printf("-warning]");	break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:		debug_printf("-error]");	break;
		//case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT:
		default: break;
	}

	debug_printf(" %s\n", cb_data->pMessage);

	// returning false means we don't abort the Vulkan call that triggered the debug callback
	return VK_FALSE;
}

static VkDebugUtilsMessengerCreateInfoEXT setup_debug_messenger_create_info(PFN_vkDebugUtilsMessengerCallbackEXT callback)
{
	VkDebugUtilsMessengerCreateInfoEXT create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

	create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

	create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

	create_info.pfnUserCallback = callback;
	create_info.pUserData = nullptr;

	return 	create_info;
}

static void print_instance_info(arena_t* arena)
{
	if (!is_running_in_debug())
	{
		return;
	}

	// print supported extensions
	{
		u32 num_exts;
		vkEnumerateInstanceExtensionProperties(nullptr, &num_exts, nullptr);

		array_t<VkExtensionProperties> instance_extensions = array_init_sized<VkExtensionProperties>(arena, num_exts);
		vkEnumerateInstanceExtensionProperties(nullptr, &num_exts, instance_extensions.data);

		debug_printf("Supported Instance Extensions\n");
		for (u32 i = 0; i < num_exts; i++)
		{
			const VkExtensionProperties& p = instance_extensions[i];
			debug_printf("  Name: %s | Spec Version: %i\n", p.extensionName, p.specVersion);
		}
	}

	// print supported validation layers
	{
		u32 num_layers;
		vkEnumerateInstanceLayerProperties(&num_layers, nullptr);

		array_t<VkLayerProperties> instance_layers = array_init_sized<VkLayerProperties>(arena, num_layers);
		vkEnumerateInstanceLayerProperties(&num_layers, instance_layers.data);

		debug_printf("Supported Instance Validation Layers\n");
		for (u32 i = 0; i < num_layers; i++)
		{
			const VkLayerProperties& l = instance_layers[i];
			debug_printf("  Name: %s | Desc: %s | Spec Version: %i\n", l.layerName, l.description, l.specVersion);
		}
	}
}

static bool check_validation_layer_support(arena_t* arena)
{
	u32 num_layers;
	vkEnumerateInstanceLayerProperties(&num_layers, nullptr);

	array_t<VkLayerProperties> instance_layers = array_init_sized<VkLayerProperties>(arena, num_layers);
	vkEnumerateInstanceLayerProperties(&num_layers, instance_layers.data);

	for (const char* layerName : VALIDATION_LAYERS)
	{
		bool layerFound = false;

		for (u32 i = 0; i < num_layers; i++)
		{
			const VkLayerProperties& l = instance_layers[i];
			if (strcmp(layerName, l.layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}

		if (!layerFound)
		{
			return false;
		}
	}

	return true;
}

static VkSampleCountFlagBits get_max_msaa_samples(VkPhysicalDeviceProperties props)
{
	VkSampleCountFlags counts = props.limits.framebufferColorSampleCounts & props.limits.framebufferDepthSampleCounts;
	if (counts & VK_SAMPLE_COUNT_64_BIT) return VK_SAMPLE_COUNT_64_BIT;
	if (counts & VK_SAMPLE_COUNT_32_BIT) return VK_SAMPLE_COUNT_32_BIT;
	if (counts & VK_SAMPLE_COUNT_16_BIT) return VK_SAMPLE_COUNT_16_BIT;
	if (counts & VK_SAMPLE_COUNT_8_BIT) return VK_SAMPLE_COUNT_8_BIT;
	if (counts & VK_SAMPLE_COUNT_4_BIT) return VK_SAMPLE_COUNT_4_BIT;
	if (counts & VK_SAMPLE_COUNT_2_BIT) return VK_SAMPLE_COUNT_2_BIT;
	return VK_SAMPLE_COUNT_1_BIT;
}

static bool check_physical_device_extension_support(arena_t* arena, VkPhysicalDevice device)
{
	u32 num_extensions = 0;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &num_extensions, nullptr);

	array_t<VkExtensionProperties> extensions = array_init_sized<VkExtensionProperties>(arena, num_extensions);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &num_extensions, extensions.data);

	u8 num_required_extensions = ARRAY_LEN(DEVICE_EXTENSIONS);
	u8 has_extension_counter = 0;

	for(u32 i = 0; i < num_extensions; i++)
	{
		const VkExtensionProperties& props = extensions[i];

		for(u32 j = 0; j < num_required_extensions; j++)
		{
			const char* extension_name = props.extensionName;
			const char* required_extension = DEVICE_EXTENSIONS[j];
			if(strcmp(extension_name, required_extension) == 0)
			{
				has_extension_counter++;
				break;
			}
		}
	}

	return has_extension_counter == num_required_extensions;
}

static bool is_physical_device_suitable(arena_t* arena, VkPhysicalDevice device, VkSurfaceKHR surface)
{
	VkPhysicalDeviceProperties props;
	vkGetPhysicalDeviceProperties(device, &props);

	VkPhysicalDeviceFeatures features;
	vkGetPhysicalDeviceFeatures(device, &features);

	// TODO: Support integrated gpus in the future
	if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
	{
		return false;
	}

	if (features.samplerAnisotropy == VK_FALSE)
	{
		return false;
	}

	bool supportsExtensions = check_physical_device_extension_support(arena, device);
	if (!supportsExtensions)
	{
		return false;
	}

	vk_swapchain_info_t swapchain_info = query_swapchain(arena, device, surface);
	if (swapchain_info.formats.cur_size == 0 || swapchain_info.present_modes.cur_size == 0)
	{
		return false;
	}

	vk_queue_indices_t queue_indices = find_queue_indices(arena, device, surface);
	return has_required_queues(queue_indices);
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
        (u32)s_queue_indices.graphics_and_compute
    };
    staging_buffer_create_info.pQueueFamilyIndices = queue_families;
    staging_buffer_create_info.queueFamilyIndexCount = ARRAY_LEN(queue_families);

    SM_VULKAN_ASSERT(vkCreateBuffer(s_device, &staging_buffer_create_info, nullptr, &staging_buffer));

    VkMemoryRequirements staging_buffer_mem_requirements{};
    vkGetBufferMemoryRequirements(s_device, staging_buffer, &staging_buffer_mem_requirements);

    VkDeviceMemory staging_buffer_memory = VK_NULL_HANDLE;

    // allocate staging memory
    VkMemoryAllocateInfo staging_alloc_info{};
    staging_alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    staging_alloc_info.pNext = nullptr;
    staging_alloc_info.allocationSize = staging_buffer_mem_requirements.size;
    staging_alloc_info.memoryTypeIndex = find_supported_memory_type(s_phys_device_mem_props,
        staging_buffer_mem_requirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    SM_VULKAN_ASSERT(vkAllocateMemory(s_device, &staging_alloc_info, nullptr, &staging_buffer_memory));
    SM_VULKAN_ASSERT(vkBindBufferMemory(s_device, staging_buffer, staging_buffer_memory, 0));

    // map staging memory and memcpy data into it
    void* gpu_staging_memory = nullptr;
    vkMapMemory(s_device, staging_buffer_memory, 0, staging_buffer_mem_requirements.size, 0, &gpu_staging_memory);
    memcpy(gpu_staging_memory, src_data, src_data_size);
    vkUnmapMemory(s_device, staging_buffer_memory);

    // transfer data to actual buffer
    VkCommandBufferAllocateInfo command_buffer_alloc_info{};
    command_buffer_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_alloc_info.pNext = nullptr;
    command_buffer_alloc_info.commandPool = s_graphics_command_pool;
    command_buffer_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_alloc_info.commandBufferCount = 1;

    VkCommandBuffer buffer_copy_command_buffer = VK_NULL_HANDLE;
    SM_VULKAN_ASSERT(vkAllocateCommandBuffers(s_device, &command_buffer_alloc_info, &buffer_copy_command_buffer));

    VkCommandBufferBeginInfo buffer_copy_command_buffer_begin_info{};
    buffer_copy_command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    buffer_copy_command_buffer_begin_info.pNext = nullptr;
    buffer_copy_command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    buffer_copy_command_buffer_begin_info.pInheritanceInfo = nullptr;
    SM_VULKAN_ASSERT(vkBeginCommandBuffer(buffer_copy_command_buffer, &buffer_copy_command_buffer_begin_info));

    VkBufferCopy buffer_copy{};
    buffer_copy.srcOffset = 0;
    buffer_copy.dstOffset = 0;
    buffer_copy.size = src_data_size;

    VkBufferCopy copy_regions[] = {
        buffer_copy
    };
    vkCmdCopyBuffer(buffer_copy_command_buffer, staging_buffer, dst_buffer, ARRAY_LEN(copy_regions), copy_regions);

    vkEndCommandBuffer(buffer_copy_command_buffer);

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = nullptr;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitDstStageMask = nullptr;
    submit_info.pWaitDstStageMask = 0;
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
    SM_VULKAN_ASSERT(vkQueueSubmit(s_graphics_queue, ARRAY_LEN(submit_infos), submit_infos, nullptr));
    vkQueueWaitIdle(s_graphics_queue);

    vkDestroyBuffer(s_device, staging_buffer, nullptr);
    vkFreeCommandBuffers(s_device, s_graphics_command_pool, ARRAY_LEN(commands_to_submit), commands_to_submit);
}

static void init_swapchain()
{
	arena_t* stack_arena;
	arena_stack_init(stack_arena, 256);
    vk_swapchain_info_t swapchain_info = query_swapchain(stack_arena, s_phys_device, s_surface);

    VkSurfaceFormatKHR swapchain_format = swapchain_info.formats[0];
    for(int i = 0; i < swapchain_info.formats.cur_size; i++)
    {
        const VkSurfaceFormatKHR& format = swapchain_info.formats[i];
        if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            swapchain_format = format;
        }
    }

    VkPresentModeKHR swapchain_present_mode = VK_PRESENT_MODE_FIFO_KHR;
    for(int i = 0; i < swapchain_info.present_modes.cur_size; i++)
    {
        const VkPresentModeKHR& mode = swapchain_info.present_modes[i];
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            swapchain_present_mode = mode;
            break;
        }
    }

    VkExtent2D swapchain_extent{};
    if (swapchain_info.capabilities.currentExtent.width != UINT32_MAX)
    {
        swapchain_extent = swapchain_info.capabilities.currentExtent;
    }
    else
    {
        swapchain_extent = { s_window->width, s_window->height };
        swapchain_extent.width = clamp(swapchain_extent.width, swapchain_info.capabilities.minImageExtent.width, swapchain_info.capabilities.maxImageExtent.width);
        swapchain_extent.height = clamp(swapchain_extent.height, swapchain_info.capabilities.minImageExtent.height, swapchain_info.capabilities.maxImageExtent.height);
    }

    u32 image_count = swapchain_info.capabilities.minImageCount + 1; // one extra image to prevent waiting on driver
    if (swapchain_info.capabilities.maxImageCount > 0)
    {
        image_count = min(image_count, swapchain_info.capabilities.maxImageCount);
    }

    // TODO: Allow fullscreen
    //VkSurfaceFullScreenExclusiveInfoEXT fullScreenInfo = {};
    //fullScreenInfo.sType = VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_INFO_EXT;
    //fullScreenInfo.pNext = nullptr;
    //fullScreenInfo.fullScreenExclusive = VK_FULL_SCREEN_EXCLUSIVE_ALLOWED_EXT;

    VkSwapchainCreateInfoKHR create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.pNext = nullptr; // use full_screen_info here
    create_info.surface = s_surface;
    create_info.minImageCount = image_count;
    create_info.imageFormat = swapchain_format.format;
    create_info.imageColorSpace = swapchain_format.colorSpace;
    create_info.presentMode = swapchain_present_mode;
    create_info.imageExtent = swapchain_extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    u32 queueFamilyIndices[] = { (u32)s_queue_indices.graphics_and_compute, (u32)s_queue_indices.presentation };

    if (s_queue_indices.graphics_and_compute != s_queue_indices.presentation)
    {
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 0;
        create_info.pQueueFamilyIndices = nullptr;
    }

    create_info.preTransform = swapchain_info.capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = VK_NULL_HANDLE;

    SM_VULKAN_ASSERT(vkCreateSwapchainKHR(s_device, &create_info, nullptr, &s_swapchain));

    u32 num_images = 0;
    vkGetSwapchainImagesKHR(s_device, s_swapchain, &num_images, nullptr);

	array_resize(s_swapchain_images, num_images);
    vkGetSwapchainImagesKHR(s_device, s_swapchain, &num_images, s_swapchain_images.data);

    s_swapchain_format = swapchain_format.format;
    s_swapchain_extent = swapchain_extent;

    VkCommandBufferAllocateInfo command_buffer_alloc_info{};
    command_buffer_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_alloc_info.pNext = nullptr;
    command_buffer_alloc_info.commandPool = s_graphics_command_pool;
    command_buffer_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_alloc_info.commandBufferCount = 1;

    VkCommandBuffer command_buffer;
    vkAllocateCommandBuffers(s_device, &command_buffer_alloc_info, &command_buffer);

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(command_buffer, &begin_info);

    // transition swapchain images to presentation layout
    for (u32 i = 0; i < (u32)s_swapchain_images.cur_size; i++)
    {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        barrier.srcAccessMask = VK_ACCESS_NONE;
        barrier.dstAccessMask = VK_ACCESS_NONE;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = s_swapchain_images[i];
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        vkCmdPipelineBarrier(command_buffer,
                             VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             0,
                             0, nullptr,
                             0, nullptr,
                             1, &barrier);
    }
    vkEndCommandBuffer(command_buffer);
    
    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    vkQueueSubmit(s_graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(s_graphics_queue);

    vkFreeCommandBuffers(s_device, s_graphics_command_pool, 1, &command_buffer);
}

static void init_render_frames_render_targets()
{
    for(size_t i = 0; i < s_render_frames.cur_size; i++)
    {
        render_frame_t& frame = s_render_frames[i];

        // main draw resources
        {
            // multisample color
            {
                frame.main_draw_color_multisample_num_mips = 1;

                // VkImage
                VkImageCreateInfo image_create_info{};
                image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                image_create_info.imageType = VK_IMAGE_TYPE_2D;
                image_create_info.extent.width = s_swapchain_extent.width;
                image_create_info.extent.height = s_swapchain_extent.height;
                image_create_info.extent.depth = 1;
                image_create_info.mipLevels = frame.main_draw_color_multisample_num_mips;
                image_create_info.arrayLayers = 1;
                image_create_info.format = s_main_color_format;
                image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
                image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                image_create_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
                image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                image_create_info.samples = s_max_msaa_samples;
                image_create_info.flags = 0;
                SM_VULKAN_ASSERT(vkCreateImage(s_device, &image_create_info, nullptr, &frame.main_draw_color_multisample_image));

                // VkDeviceMemory
                VkMemoryRequirements mem_requirements;
                vkGetImageMemoryRequirements(s_device, frame.main_draw_color_multisample_image, &mem_requirements);

                VkMemoryAllocateInfo alloc_info{};
                alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                alloc_info.allocationSize = mem_requirements.size;
                alloc_info.memoryTypeIndex = find_supported_memory_type(s_phys_device_mem_props, mem_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
                SM_VULKAN_ASSERT(vkAllocateMemory(s_device, &alloc_info, nullptr, &frame.main_draw_color_multisample_device_memory));

                vkBindImageMemory(s_device, frame.main_draw_color_multisample_image, frame.main_draw_color_multisample_device_memory, 0);

                // VkImageView
                VkImageViewCreateInfo image_view_create_info{};
                image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                image_view_create_info.image = frame.main_draw_color_multisample_image;
                image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
                image_view_create_info.format = s_main_color_format;
                image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                image_view_create_info.subresourceRange.baseMipLevel = 0;
                image_view_create_info.subresourceRange.levelCount = frame.main_draw_color_multisample_num_mips;
                image_view_create_info.subresourceRange.baseArrayLayer = 0;
                image_view_create_info.subresourceRange.layerCount = 1;
                SM_VULKAN_ASSERT(vkCreateImageView(s_device, &image_view_create_info, nullptr, &frame.main_draw_color_multisample_image_view));
            }

            // multisample depth
            {
                frame.main_draw_depth_multisample_num_mips = 1;

                // VkImage
                VkImageCreateInfo image_create_info{};
                image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                image_create_info.imageType = VK_IMAGE_TYPE_2D;
                image_create_info.extent.width = s_swapchain_extent.width;
                image_create_info.extent.height = s_swapchain_extent.height;
                image_create_info.extent.depth = 1;
                image_create_info.mipLevels = frame.main_draw_depth_multisample_num_mips;
                image_create_info.arrayLayers = 1;
                image_create_info.format = s_depth_format;
                image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
                image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                image_create_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
                image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                image_create_info.samples = s_max_msaa_samples;
                image_create_info.flags = 0;
                SM_VULKAN_ASSERT(vkCreateImage(s_device, &image_create_info, nullptr, &frame.main_draw_depth_multisample_image));

                // VkDeviceMemory
                VkMemoryRequirements mem_requirements;
                vkGetImageMemoryRequirements(s_device, frame.main_draw_depth_multisample_image, &mem_requirements);

                VkMemoryAllocateInfo alloc_info{};
                alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                alloc_info.allocationSize = mem_requirements.size;
                alloc_info.memoryTypeIndex = find_supported_memory_type(s_phys_device_mem_props, mem_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
                SM_VULKAN_ASSERT(vkAllocateMemory(s_device, &alloc_info, nullptr, &frame.main_draw_depth_multisample_device_memory));

                vkBindImageMemory(s_device, frame.main_draw_depth_multisample_image, frame.main_draw_depth_multisample_device_memory, 0);

                // VkImageView
                VkImageViewCreateInfo image_view_create_info{};
                image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                image_view_create_info.image = frame.main_draw_depth_multisample_image;
                image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
                image_view_create_info.format = s_depth_format;
                image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                image_view_create_info.subresourceRange.baseMipLevel = 0;
                image_view_create_info.subresourceRange.levelCount = frame.main_draw_depth_multisample_num_mips;
                image_view_create_info.subresourceRange.baseArrayLayer = 0;
                image_view_create_info.subresourceRange.layerCount = 1;
                SM_VULKAN_ASSERT(vkCreateImageView(s_device, &image_view_create_info, nullptr, &frame.main_draw_depth_multisample_image_view));
            }

            // color resolve
            {
                frame.main_draw_color_resolve_num_mips = 1;

                // VkImage
                VkImageCreateInfo image_create_info{};
                image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                image_create_info.imageType = VK_IMAGE_TYPE_2D;
                image_create_info.extent.width = s_swapchain_extent.width;
                image_create_info.extent.height = s_swapchain_extent.height;
                image_create_info.extent.depth = 1;
                image_create_info.mipLevels = frame.main_draw_color_resolve_num_mips;
                image_create_info.arrayLayers = 1;
                image_create_info.format = s_main_color_format;
                image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
                image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                image_create_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
                image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
                image_create_info.flags = 0;
                SM_VULKAN_ASSERT(vkCreateImage(s_device, &image_create_info, nullptr, &frame.main_draw_color_resolve_image));

                // VkDeviceMemory
                VkMemoryRequirements mem_requirements;
                vkGetImageMemoryRequirements(s_device, frame.main_draw_color_resolve_image, &mem_requirements);

                VkMemoryAllocateInfo alloc_info{};
                alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                alloc_info.allocationSize = mem_requirements.size;
                alloc_info.memoryTypeIndex = find_supported_memory_type(s_phys_device_mem_props, mem_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
                SM_VULKAN_ASSERT(vkAllocateMemory(s_device, &alloc_info, nullptr, &frame.main_draw_color_resolve_device_memory));

                vkBindImageMemory(s_device, frame.main_draw_color_resolve_image, frame.main_draw_color_resolve_device_memory, 0);

                // VkImageView
                VkImageViewCreateInfo image_view_create_info{};
                image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                image_view_create_info.image = frame.main_draw_color_resolve_image;
                image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
                image_view_create_info.format = s_main_color_format;
                image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                image_view_create_info.subresourceRange.baseMipLevel = 0;
                image_view_create_info.subresourceRange.levelCount = frame.main_draw_color_resolve_num_mips;
                image_view_create_info.subresourceRange.baseArrayLayer = 0;
                image_view_create_info.subresourceRange.layerCount = 1;
                SM_VULKAN_ASSERT(vkCreateImageView(s_device, &image_view_create_info, nullptr, &frame.main_draw_color_resolve_image_view));
            }

            {
                VkFramebufferCreateInfo create_info{};
                create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                create_info.renderPass = s_main_draw_render_pass;
                VkImageView image_views[] = {
                    frame.main_draw_color_multisample_image_view,
                    frame.main_draw_depth_multisample_image_view,
                    frame.main_draw_color_resolve_image_view
                };
                create_info.attachmentCount = ARRAY_LEN(image_views);
                create_info.pAttachments = image_views;
                create_info.width = s_swapchain_extent.width;
                create_info.height = s_swapchain_extent.height;
                create_info.layers = 1;
                SM_VULKAN_ASSERT(vkCreateFramebuffer(s_device, &create_info, nullptr, &frame.main_draw_framebuffer));
            }
        }

        // post processing
        {
            frame.post_processing_color_num_mips = 1;

            // VkImage
            VkImageCreateInfo image_create_info{};
            image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            image_create_info.imageType = VK_IMAGE_TYPE_2D;
            image_create_info.extent.width = s_swapchain_extent.width;
            image_create_info.extent.height = s_swapchain_extent.height;
            image_create_info.extent.depth = 1;
            image_create_info.mipLevels = frame.post_processing_color_num_mips;
            image_create_info.arrayLayers = 1;
            image_create_info.format = s_main_color_format;
            image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
            image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            image_create_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
            image_create_info.flags = 0;
            SM_VULKAN_ASSERT(vkCreateImage(s_device, &image_create_info, nullptr, &frame.post_processing_color_image));

            // VkDeviceMemory
            VkMemoryRequirements mem_requirements;
            vkGetImageMemoryRequirements(s_device, frame.post_processing_color_image, &mem_requirements);

            VkMemoryAllocateInfo alloc_info{};
            alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            alloc_info.allocationSize = mem_requirements.size;
            alloc_info.memoryTypeIndex = find_supported_memory_type(s_phys_device_mem_props, mem_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            SM_VULKAN_ASSERT(vkAllocateMemory(s_device, &alloc_info, nullptr, &frame.post_processing_color_device_memory));

            vkBindImageMemory(s_device, frame.post_processing_color_image, frame.post_processing_color_device_memory, 0);

            // VkImageView
            VkImageViewCreateInfo image_view_create_info{};
            image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            image_view_create_info.image = frame.post_processing_color_image;
            image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            image_view_create_info.format = s_main_color_format;
            image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            image_view_create_info.subresourceRange.baseMipLevel = 0;
            image_view_create_info.subresourceRange.levelCount = frame.post_processing_color_num_mips;
            image_view_create_info.subresourceRange.baseArrayLayer = 0;
            image_view_create_info.subresourceRange.layerCount = 1;
            SM_VULKAN_ASSERT(vkCreateImageView(s_device, &image_view_create_info, nullptr, &frame.post_processing_color_image_view));
		}

        // imgui 
        {
            VkFramebufferCreateInfo create_info{};
            create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            create_info.renderPass = s_imgui_render_pass;
            VkImageView image_views[] = {
                frame.main_draw_color_resolve_image_view
            };
            create_info.attachmentCount = ARRAY_LEN(image_views);
            create_info.pAttachments = image_views;
            create_info.width = s_swapchain_extent.width;
            create_info.height = s_swapchain_extent.height;
            create_info.layers = 1;
            SM_VULKAN_ASSERT(vkCreateFramebuffer(s_device, &create_info, nullptr, &frame.imgui_framebuffer));
        }
	}
}

static void init_render_frames()
{
    for(size_t i = 0; i < s_render_frames.cur_size; i++)
    {
        render_frame_t& frame = s_render_frames[i];

		// swapchain image is ready semaphore
        {
            VkSemaphoreCreateInfo create_info{};
            create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            SM_VULKAN_ASSERT(vkCreateSemaphore(s_device, &create_info, nullptr, &frame.swapchain_image_is_ready_semaphore));
        }

		// frame completed semaphore
        {
            VkSemaphoreCreateInfo create_info{};
            create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            SM_VULKAN_ASSERT(vkCreateSemaphore(s_device, &create_info, nullptr, &frame.frame_completed_semaphore));
        }

		// frame completed fence
        {
            VkFenceCreateInfo create_info{};
            create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
            SM_VULKAN_ASSERT(vkCreateFence(s_device, &create_info, nullptr, &frame.frame_completed_fence));
        }

		// frame command buffer
        {
            VkCommandBufferAllocateInfo alloc_info{};
            alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            alloc_info.commandBufferCount = 1;
            alloc_info.commandPool = s_graphics_command_pool;
            alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            SM_VULKAN_ASSERT(vkAllocateCommandBuffers(s_device, &alloc_info, &frame.frame_command_buffer));
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
            vkAllocateDescriptorSets(s_device, &alloc_info, &frame.frame_descriptor_set);
        }

		// frame uniform buffer
        {
            VkBufferCreateInfo create_info = {};
            create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            create_info.size = sizeof(frame_render_data_t);
            create_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            SM_VULKAN_ASSERT(vkCreateBuffer(s_device, &create_info, nullptr, &frame.frame_descriptor_buffer));

            VkMemoryRequirements mem_requirements;
            vkGetBufferMemoryRequirements(s_device, frame.frame_descriptor_buffer, &mem_requirements);

            VkMemoryAllocateInfo alloc_info{};
            alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            alloc_info.allocationSize = mem_requirements.size;
            alloc_info.memoryTypeIndex = find_supported_memory_type(s_phys_device_mem_props, mem_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            SM_VULKAN_ASSERT(vkAllocateMemory(s_device, &alloc_info, nullptr, &frame.frame_descriptor_buffer_memory));

            vkBindBufferMemory(s_device, frame.frame_descriptor_buffer, frame.frame_descriptor_buffer_memory, 0);
        }

		// frame command buffer
        {
            VkCommandBufferAllocateInfo alloc_info{};
            alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            alloc_info.commandBufferCount = 1;
            alloc_info.commandPool = s_graphics_command_pool;
            alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            SM_VULKAN_ASSERT(vkAllocateCommandBuffers(s_device, &alloc_info, &frame.frame_command_buffer));
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
            SM_VULKAN_ASSERT(vkCreateDescriptorPool(s_device, &create_info, nullptr, &frame.mesh_instance_descriptor_pool));
        }

		// post processing descriptor set
        {
            VkDescriptorSetAllocateInfo alloc_info{};
            alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            alloc_info.descriptorPool = s_frame_descriptor_pool;
            VkDescriptorSetLayout set_layouts[] = {
                s_post_process_descriptor_set_layout
            };
            alloc_info.pSetLayouts = set_layouts;
            alloc_info.descriptorSetCount = ARRAY_LEN(set_layouts);
            SM_VULKAN_ASSERT(vkAllocateDescriptorSets(s_device, &alloc_info, &frame.post_processing_descriptor_set));
        }

        // infinite grid
        {
            // uniform buffer
            {
                VkBufferCreateInfo create_info{};
                create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                create_info.flags = 0;
                create_info.size = sizeof(infinite_grid_data_t);
                create_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
                create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

                u32 queue_indices[] = {
                    (u32)s_queue_indices.graphics_and_compute
                };
                create_info.queueFamilyIndexCount = ARRAY_LEN(queue_indices);
                create_info.pQueueFamilyIndices = queue_indices;

                SM_VULKAN_ASSERT(vkCreateBuffer(s_device, &create_info, nullptr, &frame.infinite_grid_data_buffer));

                VkMemoryRequirements mem_requirements{};
                vkGetBufferMemoryRequirements(s_device, frame.infinite_grid_data_buffer, &mem_requirements);

                VkMemoryAllocateInfo alloc_info{};
                alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                alloc_info.allocationSize = mem_requirements.size;
                alloc_info.memoryTypeIndex = find_supported_memory_type(s_phys_device_mem_props, mem_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

                SM_VULKAN_ASSERT(vkAllocateMemory(s_device, &alloc_info, nullptr, &frame.infinite_grid_buffer_device_memory));

                vkBindBufferMemory(s_device, frame.infinite_grid_data_buffer, frame.infinite_grid_buffer_device_memory, 0);
                frame.infinite_grid_buffer_device_size = mem_requirements.size;
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

                SM_VULKAN_ASSERT(vkAllocateDescriptorSets(s_device, &alloc_info, &frame.infinite_grid_descriptor_set));
            }
        }
    }

	init_render_frames_render_targets();
}

static void refresh_render_frames_render_targets()
{
    for(size_t i = 0; i < s_render_frames.cur_size; i++)
    {
        render_frame_t& frame = s_render_frames[i];

		vkDestroyImageView(s_device, frame.main_draw_color_multisample_image_view, nullptr);
		vkDestroyImage(s_device, frame.main_draw_color_multisample_image, nullptr);
		vkFreeMemory(s_device, frame.main_draw_color_multisample_device_memory, nullptr);

		vkDestroyImageView(s_device, frame.main_draw_depth_multisample_image_view, nullptr);
		vkDestroyImage(s_device, frame.main_draw_depth_multisample_image, nullptr);
		vkFreeMemory(s_device, frame.main_draw_depth_multisample_device_memory, nullptr);

		vkDestroyImageView(s_device, frame.main_draw_color_resolve_image_view, nullptr);
		vkDestroyImage(s_device, frame.main_draw_color_resolve_image, nullptr);
		vkFreeMemory(s_device, frame.main_draw_color_resolve_device_memory, nullptr);

		vkDestroyFramebuffer(s_device, frame.main_draw_framebuffer, nullptr);

		vkDestroyImageView(s_device, frame.post_processing_color_image_view, nullptr);
		vkDestroyImage(s_device, frame.post_processing_color_image, nullptr);
		vkFreeMemory(s_device, frame.post_processing_color_device_memory, nullptr);

		vkDestroyFramebuffer(s_device, frame.imgui_framebuffer, nullptr);
	}

	init_render_frames_render_targets();
}

static void init_pipelines()
{
    arena_t* temp_shader_arena = arena_init(KiB(50));
    // viking room pipeline
    {
        // shaders
        shader_t* vertex_shader = arena_alloc_struct(temp_shader_arena, shader_t);
        SM_ASSERT(shader_compiler_compile(temp_shader_arena, shader_type_t::VERTEX, "simple-diffuse.vs.hlsl", "Main", &vertex_shader));

        shader_t* pixel_shader = arena_alloc_struct(temp_shader_arena, shader_t);
        SM_ASSERT(shader_compiler_compile(temp_shader_arena, shader_type_t::PIXEL, "simple-diffuse.ps.hlsl", "Main", &pixel_shader));

        VkShaderModuleCreateInfo vertex_shader_module_create_info{};
        vertex_shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        vertex_shader_module_create_info.pNext = nullptr;
        vertex_shader_module_create_info.flags = 0;
        vertex_shader_module_create_info.codeSize = vertex_shader->bytecode.cur_size;
        vertex_shader_module_create_info.pCode = (u32*)vertex_shader->bytecode.data;

        VkShaderModule vertex_shader_module = VK_NULL_HANDLE;
        SM_VULKAN_ASSERT(vkCreateShaderModule(s_device, &vertex_shader_module_create_info, nullptr, &vertex_shader_module));

        VkShaderModuleCreateInfo pixel_shader_module_create_info{};
        pixel_shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        pixel_shader_module_create_info.pNext = nullptr;
        pixel_shader_module_create_info.flags = 0;
        pixel_shader_module_create_info.codeSize = pixel_shader->bytecode.cur_size;
        pixel_shader_module_create_info.pCode = (u32*)pixel_shader->bytecode.data;

        VkShaderModule pixel_shader_module = VK_NULL_HANDLE;
        SM_VULKAN_ASSERT(vkCreateShaderModule(s_device, &pixel_shader_module_create_info, nullptr, &pixel_shader_module));

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

        VkVertexInputAttributeDescription vertex_input_attributes_descriptions[] = {
            vertex_pos_attribute_description,
            vertex_uv_attribute_description,
            vertex_color_attribute_description
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
        viewport.width = (float)s_swapchain_extent.width;
        viewport.height = (float)s_swapchain_extent.height;
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
        scissor.extent.width = s_swapchain_extent.width;
        scissor.extent.height = s_swapchain_extent.height;

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
        multisample_state.rasterizationSamples = s_max_msaa_samples;
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

        VkGraphicsPipelineCreateInfo pipeline_create_info{};
        pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_create_info.pNext = nullptr;
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
        pipeline_create_info.layout = s_viking_room_main_draw_pipeline_layout;
        pipeline_create_info.renderPass = s_main_draw_render_pass;
        pipeline_create_info.subpass = 0;
        pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
        pipeline_create_info.basePipelineIndex = 0;

        VkGraphicsPipelineCreateInfo pipeline_create_infos[] = {
            pipeline_create_info
        };
        SM_VULKAN_ASSERT(vkCreateGraphicsPipelines(s_device, VK_NULL_HANDLE, ARRAY_LEN(pipeline_create_infos), pipeline_create_infos, nullptr, &s_viking_room_main_draw_pipeline));

        vkDestroyShaderModule(s_device, vertex_shader_module, nullptr);
        vkDestroyShaderModule(s_device, pixel_shader_module, nullptr);
    }

    //// Infinite grid
    //{
    //    VulkanShaderStages shaderStages;
    //    {
    //        Shader vertShader;
    //        CompileShader(ShaderType::kVs, "infinite-grid.vs.hlsl", "Main", &vertShader);

    //        Shader pixelShader;
    //        CompileShader(ShaderType::kPs, "infinite-grid.ps.hlsl", "Main", &pixelShader);

    //        shaderStages.InitVsPs(vertShader, pixelShader);
    //    }

    //    VulkanMeshPipelineInputInfo pipelineMeshInputInfo;
    //    pipelineMeshInputInfo.Init(nullptr);

    //    VulkanPipelineState pipelineState;
    //    pipelineState.InitRasterState(VK_POLYGON_MODE_FILL, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_CULL_MODE_NONE);
    //    pipelineState.InitViewportState(0, 0, (F32)m_swapchain.m_extent.width, (F32)m_swapchain.m_extent.height, 0.0f, 1.0f, 0, 0, m_swapchain.m_extent.width, m_swapchain.m_extent.height);
    //    pipelineState.InitMultisampleState(VulkanDevice::Get()->m_maxNumMsaaSamples);
    //    pipelineState.InitDepthStencilState(true, true, VK_COMPARE_OP_LESS);
    //    pipelineState.PreInitAddColorBlendAttachment(true, VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD);
    //    pipelineState.InitColorBlendState(false);

    //    m_infiniteGridPipeline.InitGraphics(shaderStages, m_infiniteGridPipelineLayout, pipelineMeshInputInfo, pipelineState, m_mainDrawRenderPass);

    //    shaderStages.Destroy();
    //}

    //// Post Processing
    //{
    //    VulkanShaderStages shaderStage;
    //    {
    //        Shader computeShader;
    //        CompileShader(ShaderType::kCs, "post-processing.cs.hlsl", "Main", &computeShader);

    //        shaderStage.InitCs(computeShader);
    //    }

    //    m_postProcessingPipeline.InitCompute(shaderStage, m_postProcessingPipelineLayout);

    //    shaderStage.Destroy();
    //}

    arena_destroy(temp_shader_arena);
}

static void refresh_pipelines()
{
	vkDestroyPipeline(s_device, s_viking_room_main_draw_pipeline, nullptr);
	init_pipelines();
}

static void refresh_swapchain()
{
	vkQueueWaitIdle(s_graphics_queue);

	vkDestroySwapchainKHR(s_device, s_swapchain, nullptr);
	init_swapchain();

	refresh_render_frames_render_targets();

    refresh_pipelines();
}

void renderer_window_msg_handler(window_msg_type_t msg_type, u64 msg_data, void* user_args)
{
    if(msg_type == window_msg_type_t::CLOSE_WINDOW)
    {
        s_close_window = true;
    }
}

void sm::renderer_init(window_t* window)
{	
	arena_t* startup_arena = arena_init(MiB(100));

	s_window = window;
    window_add_msg_cb(s_window, renderer_window_msg_handler, nullptr);

	shader_compiler_init();
	primitive_shapes_init();

	// vk instance
	{
        vulkan_global_funcs_load();
        print_instance_info(startup_arena);

        // app info
        VkApplicationInfo app_info = {};
        app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pApplicationName = "sm workbench";
        app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.pEngineName = "sm engine";
        app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.apiVersion = VK_API_VERSION_1_3;

        // create info
        VkInstanceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.pApplicationInfo = &app_info;

        // extensions
        create_info.ppEnabledExtensionNames = INSTANCE_EXTENSIONS;
        create_info.enabledExtensionCount = ARRAY_LEN(INSTANCE_EXTENSIONS);

        // validation layers
        if (ENABLE_VALIDATION_LAYERS)
        {
            SM_ASSERT(check_validation_layer_support(startup_arena));
            create_info.ppEnabledLayerNames = VALIDATION_LAYERS;
            create_info.enabledLayerCount = ARRAY_LEN(VALIDATION_LAYERS);

            // this debug messenger debugs the actual instance creation
            VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info = setup_debug_messenger_create_info(vk_debug_func);
            create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debug_messenger_create_info;

            if (ENABLE_VALIDATION_BEST_PRACTICES)
            {
                VkValidationFeatureEnableEXT enables[] = { VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT };
                VkValidationFeaturesEXT features = {};
                features.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
                features.enabledValidationFeatureCount = 1;
                features.pEnabledValidationFeatures = enables;
                debug_messenger_create_info.pNext = &features;
            }
        }

        SM_VULKAN_ASSERT(vkCreateInstance(&create_info, nullptr, &s_instance));

        vulkan_instance_funcs_load(s_instance);

        // real debug messenger for the whole game
        if (ENABLE_VALIDATION_LAYERS)
        {
            VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info = setup_debug_messenger_create_info(vk_debug_func);
            SM_VULKAN_ASSERT(vkCreateDebugUtilsMessengerEXT(s_instance, &debug_messenger_create_info, nullptr, &s_debug_messenger));
        }
	}

	// vk surface
	{
        VkWin32SurfaceCreateInfoKHR create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        create_info.pNext = nullptr;
		HWND handle = get_handle<HWND>(s_window->handle);
        create_info.hwnd = handle;
        create_info.hinstance = GetModuleHandle(nullptr);
        SM_VULKAN_ASSERT(vkCreateWin32SurfaceKHR(s_instance, &create_info, nullptr, &s_surface));
	}

	// vk devices
	{
        // physical device
        VkPhysicalDevice selected_phy_device = VK_NULL_HANDLE;
        VkPhysicalDeviceProperties selected_phys_device_props = {};
        VkPhysicalDeviceMemoryProperties selected_phys_device_mem_props = {};
        vk_queue_indices_t queue_indices;
        VkSampleCountFlagBits max_num_msaa_samples = VK_SAMPLE_COUNT_1_BIT;

        {
            static const u8 max_num_devices = 5; // bro do you really have more than 5 graphics cards

            u32 num_devices = 0;
            vkEnumeratePhysicalDevices(s_instance, &num_devices, nullptr);
            SM_ASSERT(num_devices != 0 && num_devices <= max_num_devices);

            VkPhysicalDevice devices[max_num_devices];
            vkEnumeratePhysicalDevices(s_instance, &num_devices, devices);

            if (is_running_in_debug())
            {
                debug_printf("Physical Devices:\n");

                //for (const VkPhysicalDevice& device : devices)
                for (u8 i = 0; i < num_devices; i++)
                {
                    VkPhysicalDeviceProperties device_props;
                    vkGetPhysicalDeviceProperties(devices[i], &device_props);

                    //VkPhysicalDeviceFeatures deviceFeatures;
					//vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

                    debug_printf("%s\n", device_props.deviceName);
				}
			}

			for(const VkPhysicalDevice& device : devices)
			{
				if(is_physical_device_suitable(startup_arena, device, s_surface))
				{
					selected_phy_device = device;
					vkGetPhysicalDeviceProperties(selected_phy_device, &selected_phys_device_props);
					vkGetPhysicalDeviceMemoryProperties(selected_phy_device, &selected_phys_device_mem_props);
					queue_indices = find_queue_indices(startup_arena, device, s_surface);
					max_num_msaa_samples = get_max_msaa_samples(selected_phys_device_props);
					break;
				}
			}

			SM_ASSERT(VK_NULL_HANDLE != selected_phy_device);
		}

		// logical device
		VkDevice logical_device = VK_NULL_HANDLE;
		{
			f32 priority = 1.0f;
			i32 num_queues = 0;

			VkDeviceQueueCreateInfo queue_create_infos[3] = {};
			queue_create_infos[num_queues].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queue_create_infos[num_queues].queueFamilyIndex = queue_indices.graphics_and_compute;
			queue_create_infos[num_queues].queueCount = 1;
			queue_create_infos[num_queues].pQueuePriorities = &priority;
			num_queues++;

			queue_create_infos[num_queues].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queue_create_infos[num_queues].queueFamilyIndex = queue_indices.async_compute;
			queue_create_infos[num_queues].queueCount = 1;
			queue_create_infos[num_queues].pQueuePriorities = &priority;
			num_queues++;

			if(queue_indices.graphics_and_compute != queue_indices.presentation)
			{
				queue_create_infos[num_queues].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queue_create_infos[num_queues].queueFamilyIndex = queue_indices.presentation;
				queue_create_infos[num_queues].queueCount = 1;
				queue_create_infos[num_queues].pQueuePriorities = &priority;
				num_queues++;
			}

			VkPhysicalDeviceFeatures device_features{};
			device_features.samplerAnisotropy = VK_TRUE;

			VkDeviceCreateInfo create_info{};
			create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			create_info.pQueueCreateInfos = queue_create_infos;
			create_info.queueCreateInfoCount = num_queues;
			create_info.pEnabledFeatures = &device_features;
			create_info.enabledExtensionCount = (u32)ARRAY_LEN(DEVICE_EXTENSIONS);
			create_info.ppEnabledExtensionNames = DEVICE_EXTENSIONS;

			// validation layers are not used on devices anymore, but this is for older vulkan systems that still used it
			if(ENABLE_VALIDATION_LAYERS)
			{
				create_info.enabledLayerCount = ARRAY_LEN(VALIDATION_LAYERS);
				create_info.ppEnabledLayerNames = VALIDATION_LAYERS;
			}

			VkPhysicalDeviceVulkan13Features vk13features{}; // vulkan 1.3 features
			vk13features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
			vk13features.synchronization2 = VK_TRUE;

			create_info.pNext = &vk13features;

			SM_VULKAN_ASSERT(vkCreateDevice(selected_phy_device, &create_info, nullptr, &logical_device));

			vulkan_device_funcs_load(logical_device);
		}

		VkQueue graphics_queue = VK_NULL_HANDLE;
		VkQueue async_compute_queue = VK_NULL_HANDLE;
		VkQueue present_queue = VK_NULL_HANDLE;
		{
			vkGetDeviceQueue(logical_device, queue_indices.graphics_and_compute, 0, &graphics_queue);
			vkGetDeviceQueue(logical_device, queue_indices.async_compute, 0, &async_compute_queue);
			vkGetDeviceQueue(logical_device, queue_indices.presentation, 0, &present_queue);
		}

		s_phys_device = selected_phy_device;
		s_phys_device_props = selected_phys_device_props;
		s_phys_device_mem_props = selected_phys_device_mem_props;
		s_device = logical_device;
		s_queue_indices = queue_indices;
		s_graphics_queue = graphics_queue;
		s_async_compute_queue = async_compute_queue;
		s_present_queue = present_queue;
		s_max_msaa_samples = max_num_msaa_samples;
		s_depth_format = find_supported_depth_format(s_phys_device);
	}

	// command pool
	{
		VkCommandPoolCreateInfo create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		create_info.pNext = nullptr;
		create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		create_info.queueFamilyIndex = s_queue_indices.graphics_and_compute;
		SM_VULKAN_ASSERT(vkCreateCommandPool(s_device, &create_info, nullptr, &s_graphics_command_pool));
	}

	// swapchain
	const u32 default_num_images = 3;
    s_swapchain_images = array_init_sized<VkImage>(startup_arena, default_num_images);
	init_swapchain();

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
            create_info.maxSets = 1;
            SM_VULKAN_ASSERT(vkCreateDescriptorPool(s_device, &create_info, nullptr, &s_global_descriptor_pool));
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
            SM_VULKAN_ASSERT(vkCreateDescriptorPool(s_device, &create_info, nullptr, &s_frame_descriptor_pool));
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
            SM_VULKAN_ASSERT(vkCreateDescriptorPool(s_device, &create_info, nullptr, &s_material_descriptor_pool));
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
            SM_VULKAN_ASSERT(vkCreateDescriptorPool(s_device, &create_info, nullptr, &s_imgui_descriptor_pool));
		}
	}

	// descriptor set layouts
	{
		// global layout
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
            SM_VULKAN_ASSERT(vkCreateDescriptorSetLayout(s_device, &create_info, nullptr, &s_global_descriptor_set_layout));
		}

		// frame layout
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
            SM_VULKAN_ASSERT(vkCreateDescriptorSetLayout(s_device, &create_info, nullptr, &s_frame_descriptor_set_layout));
		}

        // infinite grid layout
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
            SM_VULKAN_ASSERT(vkCreateDescriptorSetLayout(s_device, &create_info, nullptr, &s_infinite_grid_descriptor_set_layout));
		}

        // post processing layout
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

            VkDescriptorSetLayoutBinding layout_bindings[] = {
				src_image_binding,
				dst_image_binding
            };

            VkDescriptorSetLayoutCreateInfo create_info{};
            create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            create_info.bindingCount = (u32)ARRAY_LEN(layout_bindings);
            create_info.pBindings = layout_bindings;
            SM_VULKAN_ASSERT(vkCreateDescriptorSetLayout(s_device, &create_info, nullptr, &s_post_process_descriptor_set_layout));
		}

        // materials layout
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
            SM_VULKAN_ASSERT(vkCreateDescriptorSetLayout(s_device, &create_info, nullptr, &s_material_descriptor_set_layout));
		}

        // mesh instance layout
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
            SM_VULKAN_ASSERT(vkCreateDescriptorSetLayout(s_device, &create_info, nullptr, &s_mesh_instance_descriptor_set_layout));
		}
	}

	// render passes
	{
		// main draw
		{
            VkAttachmentDescription2 main_color_attachment{};
            main_color_attachment.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
            main_color_attachment.format = s_main_color_format;
            main_color_attachment.samples = s_max_msaa_samples;
            main_color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            main_color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            main_color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            main_color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            main_color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            main_color_attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            main_color_attachment.flags = 0;

            VkAttachmentDescription2 main_depth_stencil_attachment{};
            main_depth_stencil_attachment.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
            main_depth_stencil_attachment.format = s_depth_format;
            main_depth_stencil_attachment.samples = s_max_msaa_samples;
            main_depth_stencil_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            main_depth_stencil_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            main_depth_stencil_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            main_depth_stencil_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            main_depth_stencil_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            main_depth_stencil_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            main_depth_stencil_attachment.flags = 0;

            VkAttachmentDescription2 main_color_resolve_attachment{};
            main_color_resolve_attachment.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
            main_color_resolve_attachment.format = s_main_color_format;
            main_color_resolve_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
            main_color_resolve_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            main_color_resolve_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            main_color_resolve_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            main_color_resolve_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            main_color_resolve_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            main_color_resolve_attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            main_color_resolve_attachment.flags = 0;

			VkAttachmentDescription2 render_pass_attachments[] = {
				main_color_attachment,
				main_depth_stencil_attachment,
				main_color_resolve_attachment
			};

            VkSubpassDescription2 subpass_desc{};
            subpass_desc.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2;
            subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

			VkAttachmentReference2 color_attachment_ref{};
			color_attachment_ref.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
			color_attachment_ref.attachment = 0;
			color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			color_attachment_ref.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

			VkAttachmentReference2 depth_stencil_attachment_ref{};
			depth_stencil_attachment_ref.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
			depth_stencil_attachment_ref.attachment = 1;
			depth_stencil_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			depth_stencil_attachment_ref.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

			VkAttachmentReference2 color_resolve_attachment_ref{};
			color_resolve_attachment_ref.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
			color_resolve_attachment_ref.attachment = 2;
			color_resolve_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			color_resolve_attachment_ref.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

			VkAttachmentReference2 color_attachments[] = {
				color_attachment_ref
			};

            subpass_desc.colorAttachmentCount = (u32)ARRAY_LEN(color_attachments);
			subpass_desc.pColorAttachments = color_attachments;
			subpass_desc.pDepthStencilAttachment = &depth_stencil_attachment_ref;
			subpass_desc.pResolveAttachments = &color_resolve_attachment_ref;

			VkSubpassDescription2 subpass_descs[] = {
				subpass_desc
			};

			VkSubpassDependency2 subpass_dependency{};
			subpass_dependency.sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2;
			subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
			subpass_dependency.dstSubpass = 0;
			subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
			subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
			subpass_dependency.srcAccessMask = VK_ACCESS_2_NONE;
			subpass_dependency.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
			subpass_dependency.dependencyFlags = 0;

			VkSubpassDependency2 subpass_dependencies[] = {
				subpass_dependency
			};

			VkRenderPassCreateInfo2 create_info = {};
			create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2;
			create_info.attachmentCount = (u32)ARRAY_LEN(render_pass_attachments);
			create_info.pAttachments = render_pass_attachments;
			create_info.subpassCount = (u32)ARRAY_LEN(subpass_descs);
			create_info.pSubpasses = subpass_descs;
			create_info.dependencyCount = (u32)ARRAY_LEN(subpass_dependencies);
			create_info.pDependencies = subpass_dependencies;

			SM_VULKAN_ASSERT(vkCreateRenderPass2(s_device, &create_info, nullptr, &s_main_draw_render_pass));
		}

		// imgui
		{
			VkAttachmentDescription2 color_attachment_desc{};
			color_attachment_desc.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
			color_attachment_desc.flags = 0;
			color_attachment_desc.format = s_main_color_format;
			color_attachment_desc.samples = VK_SAMPLE_COUNT_1_BIT;
			color_attachment_desc.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
			color_attachment_desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			color_attachment_desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			color_attachment_desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			color_attachment_desc.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			color_attachment_desc.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

			VkAttachmentDescription2 attachments[] = {
				color_attachment_desc
			};

			VkAttachmentReference2 color_attachment_ref{};
			color_attachment_ref.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
			color_attachment_ref.attachment = 0;
			color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			color_attachment_ref.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

			VkAttachmentReference2 color_attachment_refs[] = {
				color_attachment_ref
			};

			VkSubpassDescription2 subpass_desc{};
			subpass_desc.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2;
			subpass_desc.flags = 0;
			subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass_desc.pColorAttachments = color_attachment_refs;
			subpass_desc.colorAttachmentCount = (u32)ARRAY_LEN(color_attachment_refs);

			VkSubpassDescription2 subpass_descriptions[] = {
				subpass_desc
			};

			VkSubpassDependency2 subpass_dependency{};
			subpass_dependency.sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2;
			subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
			subpass_dependency.dstSubpass = 0;
			subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			subpass_dependency.srcAccessMask = VK_ACCESS_2_NONE;
			subpass_dependency.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
			subpass_dependency.dependencyFlags = 0;

			VkSubpassDependency2 subpass_dependencies[] = {
				subpass_dependency
			};

			VkRenderPassCreateInfo2 create_info{};
			create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2;
			create_info.pAttachments = attachments;
			create_info.attachmentCount = (u32)ARRAY_LEN(attachments);
			create_info.pSubpasses = subpass_descriptions;
			create_info.subpassCount = (u32)ARRAY_LEN(subpass_descriptions);
			create_info.pDependencies = subpass_dependencies;
			create_info.dependencyCount = (u32)ARRAY_LEN(subpass_dependencies);

			SM_VULKAN_ASSERT(vkCreateRenderPass2(s_device, &create_info, nullptr, &s_imgui_render_pass));
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

		HWND hwnd = get_handle<HWND>(s_window->handle);

        ImGui_ImplWin32_Init(hwnd);
        ImGui_ImplVulkan_LoadFunctions(imgui_vulkan_func_loader, &s_instance);
        ImGui_ImplVulkan_InitInfo init_info{};
        init_info.Instance = s_instance;
        init_info.PhysicalDevice = s_phys_device;
        init_info.Device = s_device;
        init_info.QueueFamily = s_queue_indices.graphics_and_compute;
        init_info.Queue = s_graphics_queue;
        init_info.PipelineCache = VK_NULL_HANDLE;
        init_info.DescriptorPool = s_imgui_descriptor_pool;
        init_info.Subpass = 0;
        init_info.MinImageCount = (u32)s_swapchain_images.cur_size; 
        init_info.ImageCount = (u32)s_swapchain_images.cur_size;
        init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        init_info.Allocator = VK_NULL_HANDLE;
        init_info.CheckVkResultFn = imgui_check_vulkan_result;
        ImGui_ImplVulkan_Init(&init_info, s_imgui_render_pass);

        f32 dpi_scale = ImGui_ImplWin32_GetDpiScaleForHwnd(hwnd);
        debug_printf("Setting ImGui DPI Scale to %f\n", dpi_scale);

        ImFontConfig font_cfg;
        font_cfg.SizePixels = floor(13.0f * dpi_scale);
        io.Fonts->AddFontDefault(&font_cfg);
        
        ImGui::GetStyle().ScaleAllSizes(dpi_scale);

        // Upload Fonts
        {
			VkCommandBuffer command_buffer = VK_NULL_HANDLE;

			VkCommandBufferAllocateInfo alloc_info{};
			alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            alloc_info.commandPool = s_graphics_command_pool;
			alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			alloc_info.commandBufferCount = 1;
			
			SM_VULKAN_ASSERT(vkAllocateCommandBuffers(s_device, &alloc_info, &command_buffer));

			VkCommandBufferBeginInfo begin_info{};
			begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			vkBeginCommandBuffer(command_buffer, &begin_info);
            ImGui_ImplVulkan_CreateFontsTexture(command_buffer);
			vkEndCommandBuffer(command_buffer);

            VkSubmitInfo submit_info{};
            submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers = &command_buffer;
            vkQueueSubmit(s_graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
            vkQueueWaitIdle(s_graphics_queue);

			vkFreeCommandBuffers(s_device, s_graphics_command_pool, 1, &command_buffer);
            ImGui_ImplVulkan_DestroyFontUploadObjects();
        }
	}

	// render frames
	{
		s_render_frames = array_init_sized<render_frame_t>(startup_arena, MAX_NUM_FRAMES_IN_FLIGHT);
		init_render_frames();
	}

	// resources
	{
		// global resources
		{
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

                SM_VULKAN_ASSERT(vkAllocateDescriptorSets(s_device, &alloc_info, &s_global_descriptor_set));
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
				create_info.maxLod = 12.0f;
				create_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
				create_info.unnormalizedCoordinates = VK_FALSE;
                SM_VULKAN_ASSERT(vkCreateSampler(s_device, &create_info, nullptr, &s_linear_sampler));
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
				
                vkUpdateDescriptorSets(s_device, ARRAY_LEN(descriptor_set_writes), descriptor_set_writes, 0, nullptr);
			}
		}

		// viking room
		{
			s_viking_room_mesh = mesh_init_from_obj(startup_arena, "viking_room.obj");
            size_t vertex_buffer_size = mesh_calc_vertex_buffer_size(s_viking_room_mesh);

			// vertex buffer
			{
                VkBufferCreateInfo create_info{};
                create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                create_info.pNext = nullptr;
                create_info.flags = 0;
                create_info.size = vertex_buffer_size;
                create_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
                create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

                u32 queueFamilyIndices[] = { (u32)s_queue_indices.graphics_and_compute };
                create_info.pQueueFamilyIndices = queueFamilyIndices;
                create_info.queueFamilyIndexCount = ARRAY_LEN(queueFamilyIndices);

                SM_VULKAN_ASSERT(vkCreateBuffer(s_device, &create_info, nullptr, &s_viking_room_vertex_buffer));

                VkMemoryRequirements viking_room_mem_requirements{};
                vkGetBufferMemoryRequirements(s_device, s_viking_room_vertex_buffer, &viking_room_mem_requirements);

                // allocate the memory
                VkMemoryAllocateInfo alloc_info{};
                alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                alloc_info.pNext = nullptr;
                alloc_info.allocationSize = viking_room_mem_requirements.size;
                alloc_info.memoryTypeIndex = find_supported_memory_type(s_phys_device_mem_props, viking_room_mem_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

				SM_VULKAN_ASSERT(vkAllocateMemory(s_device, &alloc_info, nullptr, &s_viking_room_vertex_buffer_memory));
				SM_VULKAN_ASSERT(vkBindBufferMemory(s_device, s_viking_room_vertex_buffer, s_viking_room_vertex_buffer_memory, 0));

				upload_buffer_data(s_viking_room_vertex_buffer, s_viking_room_mesh->vertices.data, vertex_buffer_size);
			}

			// viking room index buffer
			{
				size_t index_buffer_size = mesh_calc_index_buffer_size(s_viking_room_mesh);

				// create the buffer
				{
					VkBufferCreateInfo create_info{};
					create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
					create_info.pNext = nullptr;
					create_info.flags = 0;
					create_info.size = index_buffer_size;
					create_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
					create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

					u32 queue_family_indices[] = {
						(u32)s_queue_indices.graphics_and_compute
					};
					create_info.queueFamilyIndexCount = ARRAY_LEN(queue_family_indices);
					create_info.pQueueFamilyIndices = queue_family_indices;

					SM_VULKAN_ASSERT(vkCreateBuffer(s_device, &create_info, nullptr, &s_viking_room_index_buffer));
				}

				// allocate and bind the actual device memory to the buffer
				{
					VkMemoryRequirements index_buffer_memory_requirements{};
					vkGetBufferMemoryRequirements(s_device, s_viking_room_index_buffer, &index_buffer_memory_requirements);

					VkMemoryAllocateInfo alloc_info{};
					alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
					alloc_info.pNext = nullptr;
					alloc_info.allocationSize = index_buffer_memory_requirements.size;
					alloc_info.memoryTypeIndex = find_supported_memory_type(s_phys_device_mem_props, index_buffer_memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
					SM_VULKAN_ASSERT(vkAllocateMemory(s_device, &alloc_info, nullptr, &s_viking_room_index_buffer_memory));

					SM_VULKAN_ASSERT(vkBindBufferMemory(s_device, s_viking_room_index_buffer, s_viking_room_index_buffer_memory, 0));
				}

				upload_buffer_data(s_viking_room_index_buffer, s_viking_room_mesh->indices.data, index_buffer_size);
			}

			// viking room diffuse texture
			{
                // load image pixels
				const char* diffuse_texture_filename = "viking-room.png";
				sm::string_t full_filepath = string_init(startup_arena);
				full_filepath += TEXTURES_PATH;
				full_filepath += diffuse_texture_filename;

                int tex_width;
                int tex_height;
                int tex_channels;
                stbi_uc* pixels = stbi_load(full_filepath.c_str.data, &tex_width, &tex_height, &tex_channels, STBI_rgb_alpha);

                // calc num mips
                u32 num_mips = (u32)(std::floor(std::log2(max(tex_width, tex_height))) + 1);

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
                        (u32)s_queue_indices.graphics_and_compute
                    };
                    image_create_info.queueFamilyIndexCount = ARRAY_LEN(queue_family_indices);
                    image_create_info.pQueueFamilyIndices = queue_family_indices;
                    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

                    SM_VULKAN_ASSERT(vkCreateImage(s_device, &image_create_info, nullptr, &s_viking_room_diffuse_texture_image));
				}

				// image memory
				{

					VkMemoryRequirements image_mem_requirements;
					vkGetImageMemoryRequirements(s_device, s_viking_room_diffuse_texture_image, &image_mem_requirements);

					VkMemoryAllocateInfo alloc_info{};
					alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
					alloc_info.pNext = nullptr;
					alloc_info.allocationSize = image_mem_requirements.size;
					alloc_info.memoryTypeIndex = find_supported_memory_type(s_phys_device_mem_props, image_mem_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
					SM_VULKAN_ASSERT(vkAllocateMemory(s_device, &alloc_info, nullptr, &s_viking_room_diffuse_texture_memory));

					SM_VULKAN_ASSERT(vkBindImageMemory(s_device, s_viking_room_diffuse_texture_image, s_viking_room_diffuse_texture_memory, 0));
				}

                // allocate command buffer
                VkCommandBufferAllocateInfo command_buffer_alloc_info{};
                command_buffer_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
                command_buffer_alloc_info.pNext = nullptr;
                command_buffer_alloc_info.commandPool = s_graphics_command_pool;
                command_buffer_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
                command_buffer_alloc_info.commandBufferCount = 1;

                VkCommandBuffer command_buffer = VK_NULL_HANDLE;
                SM_VULKAN_ASSERT(vkAllocateCommandBuffers(s_device, &command_buffer_alloc_info, &command_buffer));

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
                        (u32)s_queue_indices.graphics_and_compute
                    };
                    staging_buffer_create_info.queueFamilyIndexCount = ARRAY_LEN(queue_family_indices);
                    staging_buffer_create_info.pQueueFamilyIndices = queue_family_indices;
                    SM_VULKAN_ASSERT(vkCreateBuffer(s_device, &staging_buffer_create_info, nullptr, &staging_buffer));

					VkMemoryRequirements staging_buffer_mem_requirements;
					vkGetBufferMemoryRequirements(s_device, staging_buffer, &staging_buffer_mem_requirements);

					VkMemoryAllocateInfo alloc_info{};
					alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
					alloc_info.pNext = nullptr;
					alloc_info.allocationSize = staging_buffer_mem_requirements.size;
					alloc_info.memoryTypeIndex = find_supported_memory_type(s_phys_device_mem_props, staging_buffer_mem_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
					SM_VULKAN_ASSERT(vkAllocateMemory(s_device, &alloc_info, nullptr, &staging_buffer_memory));

					SM_VULKAN_ASSERT(vkBindBufferMemory(s_device, staging_buffer, staging_buffer_memory, 0));

					void* mapped_memory = nullptr;
					SM_VULKAN_ASSERT(vkMapMemory(s_device, staging_buffer_memory, 0, image_size, 0, &mapped_memory));
					memcpy(mapped_memory, pixels, image_size);
					vkUnmapMemory(s_device, staging_buffer_memory);

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
					image_memory_barrier.image = s_viking_room_diffuse_texture_image;
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
					vkCmdCopyBufferToImage(command_buffer, staging_buffer, s_viking_room_diffuse_texture_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, ARRAY_LEN(copy_regions), copy_regions);
				}

				// generate mip maps for image and setup final image layout
				{
					// todo: generate mip maps using vkCmdBlitImage in a loop going through subresources of the image

					// transition the image to shader read
					VkImageMemoryBarrier image_memory_barrier{};
					image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
					image_memory_barrier.pNext = nullptr;
					image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
					image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
					image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
					image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
					image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
					image_memory_barrier.image = s_viking_room_diffuse_texture_image;
					image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					image_memory_barrier.subresourceRange.baseMipLevel = 0;
					image_memory_barrier.subresourceRange.levelCount = num_mips;
					image_memory_barrier.subresourceRange.baseArrayLayer = 0;
					image_memory_barrier.subresourceRange.layerCount = 1;

					vkCmdPipelineBarrier(command_buffer,
						VK_PIPELINE_STAGE_TRANSFER_BIT,
						VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
						0,
						0, nullptr,
						0, nullptr,
						1, &image_memory_barrier);
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

				SM_VULKAN_ASSERT(vkQueueSubmit(s_graphics_queue, 1, &command_submit_info, VK_NULL_HANDLE));

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
					image_view_create_info.image = s_viking_room_diffuse_texture_image;
					image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
					image_view_create_info.format = VK_FORMAT_R8G8B8A8_SRGB;
					image_view_create_info.components = components;
					image_view_create_info.subresourceRange = subresource_range;
					SM_VULKAN_ASSERT(vkCreateImageView(s_device, &image_view_create_info, nullptr, &s_viking_room_diffuse_texture_image_view));
				}
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
				SM_VULKAN_ASSERT(vkAllocateDescriptorSets(s_device, &alloc_info, &s_viking_room_material_descriptor_set));

				// update
                VkDescriptorImageInfo descriptor_set_write_image_info{};
                descriptor_set_write_image_info.sampler = VK_NULL_HANDLE;
                descriptor_set_write_image_info.imageView = s_viking_room_diffuse_texture_image_view;
                descriptor_set_write_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                VkWriteDescriptorSet descriptor_set_write{};
                descriptor_set_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptor_set_write.pNext = nullptr;
                descriptor_set_write.dstSet = s_viking_room_material_descriptor_set;
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

                vkUpdateDescriptorSets(s_device, ARRAY_LEN(ds_writes), ds_writes, 0, nullptr);
			}

			// mesh instance uniform buffer
			{
				VkBufferCreateInfo buffer_create_info{};
				buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				buffer_create_info.pNext = nullptr;
				buffer_create_info.flags = 0;
				buffer_create_info.size = sizeof(mesh_instance_render_data_t);
				buffer_create_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
				buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
				u32 queue_indices[] = {
					(u32)s_queue_indices.graphics_and_compute
				};
				buffer_create_info.queueFamilyIndexCount = ARRAY_LEN(queue_indices);
				buffer_create_info.pQueueFamilyIndices = queue_indices;
				SM_VULKAN_ASSERT(vkCreateBuffer(s_device, &buffer_create_info, nullptr, &s_viking_room_mesh_instance_buffer));

				VkMemoryRequirements buffer_memory_requirements;
				vkGetBufferMemoryRequirements(s_device, s_viking_room_mesh_instance_buffer, &buffer_memory_requirements);

				VkMemoryAllocateInfo buffer_memory_alloc_info{};
				buffer_memory_alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
				buffer_memory_alloc_info.pNext = nullptr;
				buffer_memory_alloc_info.allocationSize = buffer_memory_requirements.size;
				buffer_memory_requirements.memoryTypeBits = find_supported_memory_type(s_phys_device_mem_props, buffer_memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
				SM_VULKAN_ASSERT(vkAllocateMemory(s_device, &buffer_memory_alloc_info, nullptr, &s_viking_room_mesh_instance_buffer_memory));

				SM_VULKAN_ASSERT(vkBindBufferMemory(s_device, s_viking_room_mesh_instance_buffer, s_viking_room_mesh_instance_buffer_memory, 0));
			}
		}

		// pipeline layouts
		{
			// viking room
			{
                VkDescriptorSetLayout pipeline_descriptor_set_layouts[] = {
                    s_global_descriptor_set_layout,
                    s_frame_descriptor_set_layout,
                    s_material_descriptor_set_layout,
                    s_mesh_instance_descriptor_set_layout
                };

                VkPipelineLayoutCreateInfo pipeline_layout_create_info{};
                pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
                pipeline_layout_create_info.pNext = nullptr;
                pipeline_layout_create_info.flags = 0;
                pipeline_layout_create_info.setLayoutCount = ARRAY_LEN(pipeline_descriptor_set_layouts);
                pipeline_layout_create_info.pSetLayouts = pipeline_descriptor_set_layouts;
                pipeline_layout_create_info.pushConstantRangeCount = 0;
                pipeline_layout_create_info.pPushConstantRanges = nullptr;
                SM_VULKAN_ASSERT(vkCreatePipelineLayout(s_device, &pipeline_layout_create_info, nullptr, &s_viking_room_main_draw_pipeline_layout));
			}

            // infinite grid
			{
                VkDescriptorSetLayout pipeline_descriptor_set_layouts[] = {
					s_infinite_grid_descriptor_set_layout
                };

                VkPipelineLayoutCreateInfo pipeline_layout_create_info{};
                pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
                pipeline_layout_create_info.pNext = nullptr;
                pipeline_layout_create_info.flags = 0;
                pipeline_layout_create_info.setLayoutCount = ARRAY_LEN(pipeline_descriptor_set_layouts);
                pipeline_layout_create_info.pSetLayouts = pipeline_descriptor_set_layouts;
                pipeline_layout_create_info.pushConstantRangeCount = 0;
                pipeline_layout_create_info.pPushConstantRanges = nullptr;
                SM_VULKAN_ASSERT(vkCreatePipelineLayout(s_device, &pipeline_layout_create_info, nullptr, &s_infinite_grid_main_draw_pipeline_layout));
			}

            // post processing
			{
                VkDescriptorSetLayout pipeline_descriptor_set_layouts[] = {
                    s_post_process_descriptor_set_layout
                };

                VkPipelineLayoutCreateInfo pipeline_layout_create_info{};
                pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
                pipeline_layout_create_info.pNext = nullptr;
                pipeline_layout_create_info.flags = 0;
                pipeline_layout_create_info.setLayoutCount = ARRAY_LEN(pipeline_descriptor_set_layouts);
                pipeline_layout_create_info.pSetLayouts = pipeline_descriptor_set_layouts;
                pipeline_layout_create_info.pushConstantRangeCount = 0;
                pipeline_layout_create_info.pPushConstantRanges = nullptr;
                SM_VULKAN_ASSERT(vkCreatePipelineLayout(s_device, &pipeline_layout_create_info, nullptr, &s_post_process_pipeline_layout));
			}
		}

        init_pipelines();
	}

	debug_printf("Finished initializing vulkan renderer\n");
}

void sm::renderer_begin_frame()
{
}

void sm::renderer_update_frame(f32 ds)
{
	s_elapsed_time_seconds += ds;
	s_delta_time_seconds = ds;

	camera_update(s_main_camera, ds);

	// imgui
    if(!s_close_window && !window_is_minimized(s_window))
    {
        ::ImGui_ImplWin32_NewFrame();
        ::ImGui_ImplVulkan_NewFrame();
        ::ImGui::NewFrame();
    }
}

void sm::renderer_render_frame()
{
    if(s_close_window || window_is_minimized(s_window))
    {
        return;
    }

	//if (m_bReloadShadersRequested)
	//{
	//	m_bReloadShadersRequested = false;
	//	VulkanDevice::Get()->FlushPipe();
	//	InitPipelines();
	//}

    s_cur_frame_number++;
    s_cur_render_frame = s_cur_frame_number % MAX_NUM_FRAMES_IN_FLIGHT;
    render_frame_t& cur_render_frame = s_render_frames[s_cur_render_frame];

	// setup new frame
	{
		VkFence frame_completed_fences[] = {
			cur_render_frame.frame_completed_fence
		};
		SM_VULKAN_ASSERT(vkWaitForFences(s_device, ARRAY_LEN(frame_completed_fences), frame_completed_fences, VK_TRUE, UINT64_MAX));

		SM_VULKAN_ASSERT(vkResetFences(s_device, ARRAY_LEN(frame_completed_fences), frame_completed_fences));
		SM_VULKAN_ASSERT(vkResetCommandBuffer(cur_render_frame.frame_command_buffer, 0));
        SM_VULKAN_ASSERT(vkResetDescriptorPool(s_device, cur_render_frame.mesh_instance_descriptor_pool, 0));
		
		VkResult swapchain_image_acquisition_result = vkAcquireNextImageKHR(s_device, 
																			s_swapchain, 
																			UINT64_MAX, 
																			cur_render_frame.swapchain_image_is_ready_semaphore, 
																			VK_NULL_HANDLE, 
																			&cur_render_frame.swapchain_image_index);
		if(swapchain_image_acquisition_result == VK_SUBOPTIMAL_KHR)
		{
			refresh_swapchain();
		}
	}

    if (is_running_in_debug())
    {
        VkDebugUtilsLabelEXT queue_label{};
        queue_label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
        queue_label.pNext = nullptr;
        queue_label.pLabelName = "graphics";
        queue_label.color[0] = 1.0f;
        queue_label.color[1] = 1.0f;
        queue_label.color[2] = 0.0f;
        queue_label.color[3] = 1.0f;

        vkQueueBeginDebugUtilsLabelEXT(s_graphics_queue, &queue_label);
    }

    // update frame descriptor
    {
        frame_render_data_t frame_data{};
        frame_data.delta_time_seconds = s_delta_time_seconds;
        frame_data.elapsed_time_seconds = s_elapsed_time_seconds;

        upload_buffer_data(cur_render_frame.frame_descriptor_buffer, &frame_data, sizeof(frame_render_data_t));

        VkWriteDescriptorSet descriptor_set_write{};
        descriptor_set_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_set_write.pNext = nullptr;
        descriptor_set_write.dstSet = cur_render_frame.frame_descriptor_set;
        descriptor_set_write.dstBinding = 0;
        descriptor_set_write.dstArrayElement = 0;
        descriptor_set_write.descriptorCount = 1;
        descriptor_set_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptor_set_write.pImageInfo = nullptr;

        VkDescriptorBufferInfo buffer_info{};
        buffer_info.buffer = cur_render_frame.frame_descriptor_buffer;
        buffer_info.offset = 0;
        buffer_info.range = sizeof(frame_render_data_t);
        descriptor_set_write.pBufferInfo = &buffer_info;

        descriptor_set_write.pTexelBufferView = nullptr;

        VkWriteDescriptorSet descriptor_set_writes[] = {
            descriptor_set_write
        };
        vkUpdateDescriptorSets(s_device, ARRAY_LEN(descriptor_set_writes), descriptor_set_writes, 0, nullptr);
    }

    // actual frame work
    {
        // begin command buffer
        VkCommandBufferBeginInfo command_buffer_begin_info{};
        command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_begin_info.pNext = nullptr;
        command_buffer_begin_info.flags = 0;
        command_buffer_begin_info.pInheritanceInfo = nullptr;
        SM_VULKAN_ASSERT(vkBeginCommandBuffer(cur_render_frame.frame_command_buffer, &command_buffer_begin_info));

        // begin render pass
        {
            VkRenderPassBeginInfo main_draw_render_pass_info{};
            main_draw_render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            main_draw_render_pass_info.pNext = nullptr;
            main_draw_render_pass_info.renderPass = s_main_draw_render_pass;
            main_draw_render_pass_info.framebuffer = cur_render_frame.main_draw_framebuffer;

            VkRect2D render_area{};
            render_area.extent = s_swapchain_extent;
            render_area.offset.x = 0;
            render_area.offset.y = 0;
            main_draw_render_pass_info.renderArea = render_area;

            color_f32_t clear_color(0.0f, 1.0f, 1.0f, 1.0f);

            VkClearColorValue clear_color_value{};
            clear_color_value.float32[0] = clear_color.r;
            clear_color_value.float32[1] = clear_color.g;
            clear_color_value.float32[2] = clear_color.b;
            clear_color_value.float32[3] = clear_color.a;

            VkClearDepthStencilValue depth_stencil_clear_value{};
            depth_stencil_clear_value.depth = 1.0f;
            depth_stencil_clear_value.stencil = 0;

            VkClearValue clear_value{};
            clear_value.color = clear_color_value;
            clear_value.depthStencil = depth_stencil_clear_value;

            VkClearValue clear_values[] = {
                clear_value,
                clear_value,
                clear_value
            };
            main_draw_render_pass_info.clearValueCount = ARRAY_LEN(clear_values);
            main_draw_render_pass_info.pClearValues = clear_values;

            vkCmdBeginRenderPass(cur_render_frame.frame_command_buffer, &main_draw_render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
        }

        // main draw
        {
            mat44_t view = camera_get_view_transform(s_main_camera);
            mat44_t projection = init_perspective_proj(45.0f, 0.01f, 100.0f, (f32)s_swapchain_extent.width / (f32)s_swapchain_extent.height);
            mat44_t viking_room_model = mat44_t::IDENTITY;
            mat44_t viking_room_mvp = viking_room_model * view * projection;

            mesh_instance_render_data_t mesh_instance_render_data{};
            mesh_instance_render_data.mvp = viking_room_mvp;

            upload_buffer_data(s_viking_room_mesh_instance_buffer, &mesh_instance_render_data, sizeof(mesh_instance_render_data_t));

            // allocate and update mesh instance descriptor set
            VkDescriptorSetAllocateInfo viking_room_mesh_instance_descriptor_set_alloc_info{};
            viking_room_mesh_instance_descriptor_set_alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            viking_room_mesh_instance_descriptor_set_alloc_info.pNext = nullptr;
            viking_room_mesh_instance_descriptor_set_alloc_info.descriptorPool = cur_render_frame.mesh_instance_descriptor_pool;
            viking_room_mesh_instance_descriptor_set_alloc_info.descriptorSetCount = 1;
            viking_room_mesh_instance_descriptor_set_alloc_info.pSetLayouts = &s_mesh_instance_descriptor_set_layout;

            VkDescriptorSet viking_room_mesh_instance_descriptor_set = VK_NULL_HANDLE;
            SM_VULKAN_ASSERT(vkAllocateDescriptorSets(s_device, &viking_room_mesh_instance_descriptor_set_alloc_info, &viking_room_mesh_instance_descriptor_set));

            VkWriteDescriptorSet write_mesh_instance_descriptor_set{};
            write_mesh_instance_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write_mesh_instance_descriptor_set.pNext = nullptr;
            write_mesh_instance_descriptor_set.dstSet = viking_room_mesh_instance_descriptor_set;
            write_mesh_instance_descriptor_set.dstBinding = 0;
            write_mesh_instance_descriptor_set.dstArrayElement = 0;
            write_mesh_instance_descriptor_set.descriptorCount = 1;
            write_mesh_instance_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            write_mesh_instance_descriptor_set.pImageInfo = nullptr;

            VkDescriptorBufferInfo descriptor_buffer_info{};
            descriptor_buffer_info.buffer = s_viking_room_mesh_instance_buffer;
            descriptor_buffer_info.offset = 0;
            descriptor_buffer_info.range = sizeof(mesh_instance_render_data_t);
            write_mesh_instance_descriptor_set.pBufferInfo = &descriptor_buffer_info;

            write_mesh_instance_descriptor_set.pTexelBufferView = nullptr;

            VkWriteDescriptorSet descriptor_set_writes[] = {
                write_mesh_instance_descriptor_set
            };

            vkUpdateDescriptorSets(s_device, ARRAY_LEN(descriptor_set_writes), descriptor_set_writes, 0, nullptr);

            // bind everything needed to draw
            VkBuffer vertex_buffers[] = {
                s_viking_room_vertex_buffer
            };
            VkDeviceSize offset = 0;
            VkDeviceSize offsets[] = {
                offset
            };
            vkCmdBindVertexBuffers(cur_render_frame.frame_command_buffer, 0, 1, vertex_buffers, offsets);

            vkCmdBindIndexBuffer(cur_render_frame.frame_command_buffer, s_viking_room_index_buffer, 0, VK_INDEX_TYPE_UINT32);

            VkDescriptorSet descriptor_sets[] = {
                s_global_descriptor_set,
                cur_render_frame.frame_descriptor_set,
                s_viking_room_material_descriptor_set,
                viking_room_mesh_instance_descriptor_set
            };
            vkCmdBindDescriptorSets(cur_render_frame.frame_command_buffer,
                                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    s_viking_room_main_draw_pipeline_layout, 
                                    0, 
                                    ARRAY_LEN(descriptor_sets), descriptor_sets, 
                                    0, nullptr);

            vkCmdBindPipeline(cur_render_frame.frame_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, s_viking_room_main_draw_pipeline);

            // draw mesh
            vkCmdDrawIndexed(cur_render_frame.frame_command_buffer, (u32)s_viking_room_mesh->indices.cur_size, 1, 0, 0, 0);
        }

        vkCmdEndRenderPass(cur_render_frame.frame_command_buffer);

        // imgui
		{
			ui_render();

            VkRect2D render_area{};
            render_area.offset.x = 0;
            render_area.offset.y = 0;
            render_area.extent = s_swapchain_extent;

            VkRenderPassBeginInfo imgui_render_pass_begin_info{};
            imgui_render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            imgui_render_pass_begin_info.pNext = nullptr;
            imgui_render_pass_begin_info.renderPass = s_imgui_render_pass;
            imgui_render_pass_begin_info.framebuffer = cur_render_frame.imgui_framebuffer;
            imgui_render_pass_begin_info.renderArea = render_area;
            imgui_render_pass_begin_info.clearValueCount = 0;
            imgui_render_pass_begin_info.pClearValues = nullptr;

            VkSubpassContents subpass_contents = VK_SUBPASS_CONTENTS_INLINE;

            vkCmdBeginRenderPass(cur_render_frame.frame_command_buffer, &imgui_render_pass_begin_info, subpass_contents);

			::ImGui::Render();
			::ImDrawData* draw_data = ::ImGui::GetDrawData();
			ImGui_ImplVulkan_RenderDrawData(draw_data, cur_render_frame.frame_command_buffer);

            vkCmdEndRenderPass(cur_render_frame.frame_command_buffer);
		}

        // copy from color resolve to swapchain
        {
            // transition swapchain image from present to transfer dst
            {
                VkImageMemoryBarrier present_to_transfer_dst_barrier{};
                present_to_transfer_dst_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                present_to_transfer_dst_barrier.pNext = nullptr;
                present_to_transfer_dst_barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
                present_to_transfer_dst_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                present_to_transfer_dst_barrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                present_to_transfer_dst_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                present_to_transfer_dst_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                present_to_transfer_dst_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                present_to_transfer_dst_barrier.image = s_swapchain_images[cur_render_frame.swapchain_image_index];

                VkImageSubresourceRange subresource_range{};
                subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                subresource_range.baseMipLevel = 0;
                subresource_range.levelCount = 1;
                subresource_range.baseArrayLayer = 0;
                subresource_range.layerCount = 1;
                present_to_transfer_dst_barrier.subresourceRange = subresource_range;

                vkCmdPipelineBarrier(cur_render_frame.frame_command_buffer,
                                     VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                     0,
                                     0, nullptr,
                                     0, nullptr,
                                     1, &present_to_transfer_dst_barrier);
            }

            // blit from main draw color resolve to swapchain
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
                src_offset_max.x = s_swapchain_extent.width;
                src_offset_max.y = s_swapchain_extent.height;
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
                dst_offset_max.x = s_swapchain_extent.width;
                dst_offset_max.y = s_swapchain_extent.height;
                dst_offset_max.z = 1;

                VkImageBlit image_blit_region{};
                image_blit_region.srcSubresource = src_subresource;
                image_blit_region.srcOffsets[0] = src_offset_min;
                image_blit_region.srcOffsets[1] = src_offset_max;
                image_blit_region.dstSubresource = dst_subresource;
                image_blit_region.dstOffsets[0] = dst_offset_min;
                image_blit_region.dstOffsets[1] = dst_offset_max;

                vkCmdBlitImage(cur_render_frame.frame_command_buffer,
                               cur_render_frame.main_draw_color_resolve_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               s_swapchain_images[cur_render_frame.swapchain_image_index], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
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
                transfer_dst_to_present_barrier.image = s_swapchain_images[cur_render_frame.swapchain_image_index];

                VkImageSubresourceRange subresource_range{};
                subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                subresource_range.baseMipLevel = 0;
                subresource_range.levelCount = 1;
                subresource_range.baseArrayLayer = 0;
                subresource_range.layerCount = 1;
                transfer_dst_to_present_barrier.subresourceRange = subresource_range;

                vkCmdPipelineBarrier(cur_render_frame.frame_command_buffer,
                                     VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
                                     0,
                                     0, nullptr,
                                     0, nullptr,
                                     1, &transfer_dst_to_present_barrier);
            }
        }

        vkEndCommandBuffer(cur_render_frame.frame_command_buffer);

        // submit frame command buffer
        VkSubmitInfo frame_command_submit_info{};
        {
            frame_command_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            frame_command_submit_info.pNext = nullptr;

            VkSemaphore wait_semaphores[] = { cur_render_frame.swapchain_image_is_ready_semaphore };
            VkPipelineStageFlags wait_dst_stage_masks[] = { VK_PIPELINE_STAGE_TRANSFER_BIT };

            frame_command_submit_info.waitSemaphoreCount = ARRAY_LEN(wait_semaphores);
            frame_command_submit_info.pWaitSemaphores = wait_semaphores;
            frame_command_submit_info.pWaitDstStageMask = wait_dst_stage_masks;

            VkCommandBuffer command_buffers[] = {
                cur_render_frame.frame_command_buffer
            };
            frame_command_submit_info.commandBufferCount = ARRAY_LEN(command_buffers);
            frame_command_submit_info.pCommandBuffers = command_buffers;

            VkSemaphore signal_semaphores[] = {
                cur_render_frame.frame_completed_semaphore
            };
            frame_command_submit_info.signalSemaphoreCount = ARRAY_LEN(signal_semaphores);
            frame_command_submit_info.pSignalSemaphores = signal_semaphores;
        }

        VkSubmitInfo frame_command_submits[] = {
            frame_command_submit_info
        };
        vkQueueSubmit(s_graphics_queue, ARRAY_LEN(frame_command_submits), frame_command_submits, cur_render_frame.frame_completed_fence);

        // present swapchain to screen
        VkPresentInfoKHR present_info{};
        {
            present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            present_info.pNext = nullptr;
            VkSemaphore wait_semaphores[] = {
                cur_render_frame.frame_completed_semaphore
            };
            present_info.waitSemaphoreCount = ARRAY_LEN(wait_semaphores);
            present_info.pWaitSemaphores = wait_semaphores;
            present_info.swapchainCount = 1;
            present_info.pSwapchains = &s_swapchain;
            present_info.pImageIndices = &cur_render_frame.swapchain_image_index;
            present_info.pResults = nullptr;
        }
        VkResult present_result = vkQueuePresentKHR(s_graphics_queue, &present_info);
        if (present_result == VK_SUBOPTIMAL_KHR || present_result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            refresh_swapchain();
        }
        else
        {
            SM_VULKAN_ASSERT(present_result);
        }
    }

	//// setup frame commands
	//{
    //    VulkanRenderFrame& curRenderFrame = m_renderFrames[m_currentFrame];

    //    VulkanCommands::Begin(curRenderFrame.m_frameCommandBuffer, 0);

    //    // Main Draw
    //    VulkanCommands::BeginDebugLabel(curRenderFrame.m_frameCommandBuffer, "Main Draw", ColorF32(1.0f, 0.0f, 1.0f, 1.0f));
    //    {
    //        VkClearValue colorClear = {};
    //        colorClear.color = { CLEAR_COLOR.r, CLEAR_COLOR.g, CLEAR_COLOR.b, CLEAR_COLOR.a };
    //        VkClearValue depthClear = {};
    //        depthClear.depthStencil = { 1.0f, 0 };

    //        std::vector<VkClearValue> mainDrawClearValues = { colorClear, depthClear, colorClear };

    //        VkOffset2D mainDrawOffset = { 0, 0 };
    //        VkExtent2D mainDrawExtent = m_swapchain.m_extent;

    //        VulkanCommands::BeginRenderPass(curRenderFrame.m_frameCommandBuffer, m_mainDrawRenderPass.m_renderPassHandle, curRenderFrame.m_mainDrawFramebuffer.m_framebufferHandle, mainDrawOffset, mainDrawExtent, mainDrawClearValues);
    //            Mat44 view = m_pMainCamera->GetViewTransform();
    //            Mat44 projection = Mat44::CreatePerspectiveProjection(45.0f, 0.01f, 100.0f, (F32)m_swapchain.m_extent.width / (F32)m_swapchain.m_extent.height);

    //            // Viking Room
    //            VulkanCommands::InsertDebugLabel(curRenderFrame.m_frameCommandBuffer, "Viking Room", ColorF32(1.0f, 0.0f, 1.0f, 1.0f));
    //            {
    //                Mat44 model = Mat44::IDENTITY;

    //                MeshInstanceRenderData meshInstanceRenderData;
    //                meshInstanceRenderData.m_mvp = model * view * projection;
    //                //meshInstanceRenderData.m_mvp.Transpose();
    //                m_vikingRoomMeshInstanceBuffer.Update(m_graphicsCommandPool, &meshInstanceRenderData, 0);

    //                // Mesh instance descriptor set
    //                VkDescriptorSet meshInstanceDescriptorSet = curRenderFrame.m_meshInstanceDescriptorPool.AllocateSet(m_meshInstanceDescriptorSetLayout);
    //                VulkanDescriptorSetWriter meshInstanceDescriptorWriter;
    //                meshInstanceDescriptorWriter.AddUniformBufferWrite(meshInstanceDescriptorSet, m_vikingRoomMeshInstanceBuffer, 0, 0, 0, 1);
    //                meshInstanceDescriptorWriter.PerformWrites();

    //                // Bind vertex buffer
    //                VkBuffer vertexBuffer[] = { m_vikingRoomVertexBuffer.m_bufferHandle };
    //                VkDeviceSize offsets[] = { 0 };
    //                vkCmdBindVertexBuffers(curRenderFrame.m_frameCommandBuffer, 0, 1, vertexBuffer, offsets);

    //                // Bind index buffer
    //                vkCmdBindIndexBuffer(curRenderFrame.m_frameCommandBuffer, m_vikingRoomIndexBuffer.m_bufferHandle, 0, VK_INDEX_TYPE_UINT32);

    //                // Bind pipeline
    //                vkCmdBindPipeline(curRenderFrame.m_frameCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vikingRoomMainDrawPipeline.m_pipelineHandle);

    //                std::vector<VkDescriptorSet> mainDrawDescriptorSets = {
    //                    m_globalDescriptorSet,
    //                    curRenderFrame.m_frameDescriptorSet,
    //                    m_vikingRoomMaterialDS,
    //                    meshInstanceDescriptorSet	
    //                };
    //                vkCmdBindDescriptorSets(curRenderFrame.m_frameCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vikingRoomMainDrawPipelineLayout.m_layoutHandle, 0, (U32)mainDrawDescriptorSets.size(), mainDrawDescriptorSets.data(), 0, nullptr);

    //                // Draw command
    //                vkCmdDrawIndexed(curRenderFrame.m_frameCommandBuffer, (U32)m_pVikingRoomMesh->m_indices.size(), 1, 0, 0, 0);
    //            }

    //            // Infinite grid
    //            if (GetRenderSettings()->m_bDrawDebugWorldGrid)
    //            {
    //                VulkanCommands::InsertDebugLabel(curRenderFrame.m_frameCommandBuffer, "Infinite Grid", ColorF32(1.0f, 0.0f, 1.0f, 1.0f));
    //                {
	//					InfiniteGridData gridData;
	//					gridData.m_viewProjection = view * projection;
	//					gridData.m_inverseViewProjection = projection.Inversed() * view.Inversed();
	//					gridData.m_fadeDistance = GetRenderSettings()->m_debugGridFadeDistance;
	//					gridData.m_majorLineThickness = GetRenderSettings()->m_debugGridMajorLineThickness;
	//					gridData.m_minorLineThickness = GetRenderSettings()->m_debugGridMinorLineThickness;

	//					curRenderFrame.m_infiniteGridDataBuffer.Update(m_graphicsCommandPool, &gridData, 0);

	//					VulkanDescriptorSetWriter writer;
	//					writer.AddUniformBufferWrite(curRenderFrame.m_infiniteGridDescriptorSet, curRenderFrame.m_infiniteGridDataBuffer, 0, 0, 0, 1);
	//					writer.PerformWrites();

	//					vkCmdBindPipeline(curRenderFrame.m_frameCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_infiniteGridPipeline.m_pipelineHandle);

	//					std::vector<VkDescriptorSet> infiniteGridDescriptorSets = {
	//						curRenderFrame.m_infiniteGridDescriptorSet
	//					};
	//					vkCmdBindDescriptorSets(curRenderFrame.m_frameCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_infiniteGridPipelineLayout.m_layoutHandle, 0, (U32)infiniteGridDescriptorSets.size(), infiniteGridDescriptorSets.data(), 0, nullptr);

	//					// Draw command
	//					vkCmdDraw(curRenderFrame.m_frameCommandBuffer, 6, 1, 0, 0);
	//				}
	//			}
	//			VulkanCommands::EndRenderPass(curRenderFrame.m_frameCommandBuffer);

	//	}
	//	VulkanCommands::EndDebugLabel(curRenderFrame.m_frameCommandBuffer);

	//	// Post Processing
	//	VulkanCommands::BeginDebugLabel(curRenderFrame.m_frameCommandBuffer, "Post Processing", ColorF32(1.0f, 0.5f, 0.0f, 1.0f));
	//	{
	//		// Write descriptor set
	//		// Bind Descriptor set
	//		// Bind Pipeline
	//		// Execute
	//	}
	//	VulkanCommands::EndDebugLabel(curRenderFrame.m_frameCommandBuffer);

	//	// ImGui
	//	VulkanCommands::BeginDebugLabel(curRenderFrame.m_frameCommandBuffer, "ImGui", ColorF32(1.0f, 0.0f, 0.0f, 1.0f));
	//	{
	//		UI::Render();
	//		VkOffset2D offset = {0, 0};
	//		VulkanCommands::BeginRenderPass(curRenderFrame.m_frameCommandBuffer, m_imguiRenderPass.m_renderPassHandle, curRenderFrame.m_imguiFramebuffer.m_framebufferHandle, offset, m_swapchain.m_extent);
	//		::ImGui::Render();
	//		::ImDrawData* drawData = ::ImGui::GetDrawData();
	//		ImGui_ImplVulkan_RenderDrawData(drawData, curRenderFrame.m_frameCommandBuffer);
	//		VulkanCommands::EndRenderPass(curRenderFrame.m_frameCommandBuffer);
	//	}
	//	VulkanCommands::EndDebugLabel(curRenderFrame.m_frameCommandBuffer);

	//	// Copy from main draw target to swapchain
	//	VulkanCommands::BeginDebugLabel(curRenderFrame.m_frameCommandBuffer, "Copy to Swap Chain", ColorF32(0.0f, 1.0f, 0.0f, 1.0f));
	//	{
	//		VulkanCommands::TransitionImageLayout(curRenderFrame.m_frameCommandBuffer, m_swapchain.m_images[curRenderFrame.m_swapchainImageIndex], 1,
	//			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	//			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
	//			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT);

	//		VulkanCommands::BlitColorImage(curRenderFrame.m_frameCommandBuffer, curRenderFrame.m_mainDrawColorResolveTexture.m_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_swapchain.m_extent.width, m_swapchain.m_extent.height, 1, 0,
	//			m_swapchain.m_images[curRenderFrame.m_swapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_swapchain.m_extent.width, m_swapchain.m_extent.height, 1, 0);

	//		VulkanCommands::TransitionImageLayout(curRenderFrame.m_frameCommandBuffer, m_swapchain.m_images[curRenderFrame.m_swapchainImageIndex], 1,
	//			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
	//			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_NONE,
	//			VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_NONE);
	//	}
	//	VulkanCommands::EndDebugLabel(curRenderFrame.m_frameCommandBuffer);

	//	VulkanCommands::End(curRenderFrame.m_frameCommandBuffer);

	//	//if (IsDebug())
	//	//{
	//	//	VkDebugUtilsLabelEXT queueLabel = {};
	//	//	queueLabel.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
	//	//	queueLabel.pNext = nullptr;
	//	//	queueLabel.pLabelName = "Graphics Queue";
	//	//	queueLabel.color[0] = 1.0f;
	//	//	queueLabel.color[1] = 1.0f;
	//	//	queueLabel.color[2] = 0.0f;
	//	//	queueLabel.color[3] = 1.0f;

	//	//	vkQueueBeginDebugUtilsLabelEXT(VulkanDevice::Get()->m_graphicsQueue, &queueLabel);
	//	//}

	//	VkSubmitInfo submitInfo = {};
	//	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	//	submitInfo.pNext = nullptr;
	//	submitInfo.waitSemaphoreCount = 1;
	//	submitInfo.pWaitSemaphores = &curRenderFrame.m_swapchainImageIsReadySemaphore.m_semaphoreHandle;
	//	VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_TRANSFER_BIT};
	//	submitInfo.pWaitDstStageMask = waitStages;
	//	submitInfo.commandBufferCount = 1;
	//	submitInfo.pCommandBuffers = &curRenderFrame.m_frameCommandBuffer;
	//	submitInfo.signalSemaphoreCount = 1;
	//	submitInfo.pSignalSemaphores = &curRenderFrame.m_frameCompletedSemaphore.m_semaphoreHandle;
	//	vkQueueSubmit(VulkanDevice::Get()->m_graphicsQueue, 1, &submitInfo, curRenderFrame.m_frameCompletedFence.m_fenceHandle);
	//}

	//// present final image
	//{
    //    //if (m_pWindow->m_bWasClosed)
    //    //{
    //    //    return;
    //    //}

    //    VkPresentInfoKHR presentInfo = {};
    //    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    //    presentInfo.pNext = nullptr;
    //    presentInfo.waitSemaphoreCount = 1;
    //    presentInfo.pWaitSemaphores = &m_renderFrames[m_currentFrame].m_frameCompletedSemaphore.m_semaphoreHandle;
    //    presentInfo.swapchainCount = 1;
    //    presentInfo.pSwapchains = &m_swapchain.m_swapchainHandle;
    //    presentInfo.pImageIndices = &m_renderFrames[m_currentFrame].m_swapchainImageIndex;
    //    presentInfo.pResults = nullptr;
    //    VkResult presentResult = vkQueuePresentKHR(VulkanDevice::Get()->m_graphicsQueue, &presentInfo);
    //    if (m_swapchain.NeedsRefresh(presentResult))
    //    {
    //        RefreshSwapchain();
    //    }
    //    else
    //    {
    //        SM_VULKAN_ASSERT_OLD(presentResult);
    //    }
	//}

	// temp just so I can run
	ImGui::Render();
}
