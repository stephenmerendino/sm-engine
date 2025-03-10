#include "sm/render/vk_renderer.h"
#include "sm/config.h"
#include "sm/core/debug.h"
#include "sm/core/helpers.h"
#include "sm/math/helpers.h"
#include "sm/math/mat44.h"
#include "sm/render/mesh.h"
#include "sm/render/window.h"
#include "sm/render/vk_include.h"

#include "third_party/imgui/imgui.h"
#include "third_party/imgui/backends/imgui_impl_win32.h"
#include "third_party/imgui/backends/imgui_impl_vulkan.h"

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
	i32	swapchain_image_index = -1;
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

mesh_t* s_viking_room_mesh = nullptr;
VkBuffer s_viking_room_vertex_buffer = VK_NULL_HANDLE;
VkBuffer s_viking_room_index_buffer = VK_NULL_HANDLE;
//VkImage s_viking_room_diffuse_texture_image;
//VkImageView s_viking_room_diffuse_texture_image_view;
//VkDeviceMemory s_viking_room_diffuse_texture_device_memory;

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

static VkCommandPool init_command_pool(VkDevice device, const vk_queue_indices_t& queue_indices, VkQueueFlags requested_queues, VkCommandPoolCreateFlags create_flags)
{
	VkCommandPoolCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	create_info.flags = create_flags;

	// graphics + compute requested (normal queue)
	if (requested_queues & VK_QUEUE_GRAPHICS_BIT)
	{
		create_info.queueFamilyIndex = queue_indices.graphics_and_compute;
	}

	// only Compute requested (async compute)
	if ((requested_queues & (VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT)) == VK_QUEUE_COMPUTE_BIT)
	{
		create_info.queueFamilyIndex = queue_indices.async_compute;
	}

	VkCommandPool command_pool{};
	SM_VULKAN_ASSERT(vkCreateCommandPool(device, &create_info, nullptr, &command_pool));
	return command_pool;
}

static VkCommandBuffer allocate_command_buffer(VkDevice device, VkCommandPool command_pool, VkCommandBufferLevel level)
{
	VkCommandBufferAllocateInfo alloc_info{};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.level = level;
	alloc_info.commandPool = command_pool;
	alloc_info.commandBufferCount = 1;

	VkCommandBuffer command_buffer;
	vkAllocateCommandBuffers(device, &alloc_info, &command_buffer);

	return command_buffer;
}

static vk_queue_indices_t find_queue_indices(arena_t* arena, VkPhysicalDevice device, VkSurfaceKHR surface)
{
	u32 queue_familiy_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_familiy_count, nullptr);

	array_t<VkQueueFamilyProperties> queue_family_props = init_array_sized<VkQueueFamilyProperties>(arena, queue_familiy_count);
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
		details.formats = init_array_sized<VkSurfaceFormatKHR>(arena, num_surface_formats);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &num_surface_formats, details.formats.data);
	}

	// present modes
	u32 num_present_modes = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &num_present_modes, nullptr);

	if (num_present_modes != 0)
	{
		details.present_modes = init_array_sized<VkPresentModeKHR>(arena, num_present_modes);
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
	if (!is_debug())
	{
		return;
	}

	// print supported extensions
	{
		u32 num_exts;
		vkEnumerateInstanceExtensionProperties(nullptr, &num_exts, nullptr);

		array_t<VkExtensionProperties> instance_extensions = init_array_sized<VkExtensionProperties>(arena, num_exts);
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

		array_t<VkLayerProperties> instance_layers = init_array_sized<VkLayerProperties>(arena, num_layers);
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

	array_t<VkLayerProperties> instance_layers = init_array_sized<VkLayerProperties>(arena, num_layers);
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

	array_t<VkExtensionProperties> extensions = init_array_sized<VkExtensionProperties>(arena, num_extensions);
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

static void CheckImGuiVulkanResult(VkResult result)
{
    SM_VULKAN_ASSERT(result);
}

static PFN_vkVoidFunction imgui_vulkan_func_loader(const char* functionName, void* userData)
{
	VkInstance instance = *((VkInstance*)userData);
    return vkGetInstanceProcAddr(instance, functionName);
}

void sm::init_renderer(window_t* window)
{	
	s_window = window;
	init_primitive_shapes();

	arena_t* startup_arena = init_arena(MiB(100));

	// vk instance
	{
        load_vulkan_global_funcs();
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

        load_vulkan_instance_funcs(s_instance);

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

            if (is_debug())
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

			load_vulkan_device_funcs(logical_device);
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
		s_graphics_command_pool = init_command_pool(s_device, 
													s_queue_indices, 
													VK_QUEUE_GRAPHICS_BIT, 
													VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	}

	// swapchain
	{
        vk_swapchain_info_t swapchain_info = query_swapchain(startup_arena, s_phys_device, s_surface);

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

        s_swapchain_images = init_array_sized<VkImage>(startup_arena, num_images);
        vkGetSwapchainImagesKHR(s_device, s_swapchain, &num_images, s_swapchain_images.data);

        s_swapchain_format = swapchain_format.format;
		s_swapchain_extent = swapchain_extent;
        //m_imageInFlightFences.resize(m_numImages);

		VkCommandBuffer command_buffer = allocate_command_buffer(s_device, s_graphics_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
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
        init_info.CheckVkResultFn = CheckImGuiVulkanResult;
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
		s_render_frames = init_array_sized<render_frame_t>(startup_arena, MAX_NUM_FRAMES_IN_FLIGHT);

		for(size_t i = 0; i < s_render_frames.cur_size; i++)
		{
			render_frame_t& frame = s_render_frames[i];

			{
				VkSemaphoreCreateInfo create_info{};
				create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
				SM_VULKAN_ASSERT(vkCreateSemaphore(s_device, &create_info, nullptr, &frame.swapchain_image_is_ready_semaphore));
			}

			{
				VkSemaphoreCreateInfo create_info{};
				create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
				SM_VULKAN_ASSERT(vkCreateSemaphore(s_device, &create_info, nullptr, &frame.frame_completed_semaphore));
			}

			{
				VkFenceCreateInfo create_info{};
				create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
				create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
				SM_VULKAN_ASSERT(vkCreateFence(s_device, &create_info, nullptr, &frame.frame_completed_fence));
			}

			{
				VkCommandBufferAllocateInfo alloc_info{};
				alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				alloc_info.commandBufferCount = 1;
				alloc_info.commandPool = s_graphics_command_pool;
				alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
				SM_VULKAN_ASSERT(vkAllocateCommandBuffers(s_device, &alloc_info, &frame.frame_command_buffer));
			}

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

			{
                VkBufferCreateInfo create_info = {};
                create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                create_info.size = sizeof(frame_render_data_t);
                create_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
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

            //VkCommandBuffer frame_command_buffer;
			{
				VkCommandBufferAllocateInfo alloc_info{};
				alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				alloc_info.commandBufferCount = 1;
				alloc_info.commandPool = s_graphics_command_pool;
				alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
				SM_VULKAN_ASSERT(vkAllocateCommandBuffers(s_device, &alloc_info, &frame.frame_command_buffer));
			}

            // main draw resources
			{
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
					image_create_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
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

			// mesh instance descriptors
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

            // post processing
			{
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
			s_viking_room_mesh = init_from_obj(startup_arena, "viking_room.obj");

			VkBufferCreateInfo create_info{};
			create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			create_info.pNext = nullptr;
			create_info.flags = 0;
			create_info.size = calc_mesh_vertex_buffer_size(s_viking_room_mesh);
			create_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			u32 queueFamilyIndices[] = { s_queue_indices.graphics_and_compute };
			create_info.pQueueFamilyIndices = queueFamilyIndices;
			create_info.queueFamilyIndexCount = ARRAY_LEN(queueFamilyIndices);

			SM_VULKAN_ASSERT(vkCreateBuffer(s_device, &create_info, nullptr, &s_viking_room_vertex_buffer));

            //m_vikingRoomVertexBuffer.Init(VulkanBuffer::Type::kVertexBuffer, m_pVikingRoomMesh->CalcVertexBufferSize());
            //m_vikingRoomVertexBuffer.Update(m_graphicsCommandPool, m_pVikingRoomMesh->m_vertices.data(), 0);

            //m_vikingRoomIndexBuffer.Init(VulkanBuffer::Type::kIndexBuffer, m_pVikingRoomMesh->CalcIndexBufferSize());
            //m_vikingRoomIndexBuffer.Update(m_graphicsCommandPool, m_pVikingRoomMesh->m_indices.data(), 0);

            //m_vikingRoomDiffuseTexture.InitFromFile(m_graphicsCommandPool, "viking-room.png");

            //m_vikingRoomMaterialDS = m_materialDescriptorPool.AllocateSet(m_materialDescriptorSetLayout);

            //VulkanDescriptorSetWriter dsWriter;
            //dsWriter.AddSampledImageWrite(m_vikingRoomMaterialDS, m_vikingRoomDiffuseTexture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 0, 1);
            //dsWriter.PerformWrites();

            //m_vikingRoomMeshInstanceBuffer.Init(VulkanBuffer::Type::kUniformBuffer, sizeof(MeshInstanceRenderData));
		}

		// pipeline layouts
		{
            //// Viking Room
            //std::vector<VkDescriptorSetLayout> pipelineDescriptorSetLayouts = {
            //    m_globalDescriptorSetLayout.m_layoutHandle,
            //    m_frameDescriptorSetLayout.m_layoutHandle,
            //    m_materialDescriptorSetLayout.m_layoutHandle,
            //    m_meshInstanceDescriptorSetLayout.m_layoutHandle
            //};

            //m_vikingRoomMainDrawPipelineLayout.Init(pipelineDescriptorSetLayouts);

            //// Infinite Grid
            //std::vector<VkDescriptorSetLayout> infiniteGridDescriptorSetLayouts = { m_infiniteGridDescriptorSetLayout.m_layoutHandle };
            //m_infiniteGridPipelineLayout.Init(infiniteGridDescriptorSetLayouts);

            //// Post Processing
            //std::vector<VkDescriptorSetLayout> layouts = { m_postProcessingDescriptorSetLayout.m_layoutHandle };
            //m_postProcessingPipelineLayout.Init(layouts);
		}

		// pipelines
		{
            //// Viking Room Pipeline
            //{
            //    VulkanShaderStages shaderStages;
            //    {
            //        Shader vertShader;
            //        CompileShader(ShaderType::kVs, "simple-diffuse.vs.hlsl", "Main", &vertShader);

            //        Shader pixelShader;
            //        CompileShader(ShaderType::kPs, "simple-diffuse.ps.hlsl", "Main", &pixelShader);

            //        shaderStages.InitVsPs(vertShader, pixelShader);
            //    }

            //    VulkanMeshPipelineInputInfo pipelineMeshInputInfo;
            //    pipelineMeshInputInfo.Init(m_pVikingRoomMesh, false);

            //    VulkanPipelineState pipelineState;
            //    pipelineState.InitRasterState(VK_POLYGON_MODE_FILL, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_CULL_MODE_BACK_BIT);
            //    pipelineState.InitViewportState(0, 0, (F32)m_swapchain.m_extent.width, (F32)m_swapchain.m_extent.height, 0.0f, 1.0f, 0, 0, m_swapchain.m_extent.width, m_swapchain.m_extent.height);
            //    pipelineState.InitMultisampleState(VulkanDevice::Get()->m_maxNumMsaaSamples);
            //    pipelineState.InitDepthStencilState(true, true, VK_COMPARE_OP_LESS);
            //    pipelineState.PreInitAddColorBlendAttachment(false);
            //    pipelineState.InitColorBlendState(false);

            //    m_vikingRoomMainDrawPipeline.InitGraphics(shaderStages, m_vikingRoomMainDrawPipelineLayout, pipelineMeshInputInfo, pipelineState, m_mainDrawRenderPass);

            //    shaderStages.Destroy();
            //}

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
		}
	}

	debug_printf("Finished initializing vulkan renderer\n");
}