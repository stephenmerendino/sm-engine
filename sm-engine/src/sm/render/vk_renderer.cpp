#include "sm/render/vk_renderer.h"
#include "sm/config.h"
#include "sm/core/debug.h"
#include "sm/core/helpers.h"
#include "sm/math/helpers.h"
#include "sm/render/window.h"
#include "sm/render/vk_include.h"

using namespace sm;

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
	static_array_t<VkSurfaceFormatKHR> formats;
	static_array_t<VkPresentModeKHR> present_modes;
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
VkSampleCountFlagBits s_max_msaa_samples = VK_SAMPLE_COUNT_1_BIT;
VkCommandPool s_graphics_command_pool;
VkFormat s_main_color_format = VK_FORMAT_R8G8B8A8_UNORM;
VkFormat s_depth_format = VK_FORMAT_UNDEFINED;
VkSwapchainKHR s_swapchain = VK_NULL_HANDLE;
static_array_t<VkImage> s_swapchain_images;
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

static VkFormat find_supported_format(VkPhysicalDevice phys_device, const static_array_t<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	return find_supported_format(phys_device, candidates.data, (u32)candidates.size, tiling, features);
}

static VkFormat find_supported_depth_format(VkPhysicalDevice phys_device)
{
	VkFormat possibles[] = { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
	VkFormat depthFormat = find_supported_format(phys_device, possibles, ARRAY_LEN(possibles), VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	SM_ASSERT(depthFormat != VK_FORMAT_UNDEFINED);
	return depthFormat;
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

static static_array_t<VkCommandBuffer> allocate_command_buffers(arena_t& arena, VkDevice device, VkCommandPool command_pool, VkCommandBufferLevel level, u32 num_buffers)
{
	static_array_t<VkCommandBuffer> buffers = init_static_array<VkCommandBuffer>(arena, num_buffers);

	VkCommandBufferAllocateInfo alloc_info{};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.commandPool = command_pool;
	alloc_info.level = level;
	alloc_info.commandBufferCount = num_buffers;

	SM_VULKAN_ASSERT(vkAllocateCommandBuffers(device, &alloc_info, buffers.data));
	return buffers;
}

static vk_queue_indices_t find_queue_indices(arena_t& arena, VkPhysicalDevice device, VkSurfaceKHR surface)
{
	u32 queue_familiy_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_familiy_count, nullptr);

	static_array_t<VkQueueFamilyProperties> queue_family_props = init_static_array<VkQueueFamilyProperties>(arena, queue_familiy_count);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_familiy_count, queue_family_props.data);

	vk_queue_indices_t indices;

	for (int i = 0; i < (int)queue_family_props.size; i++)
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

static vk_swapchain_info_t query_swapchain(arena_t& arena, VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
	vk_swapchain_info_t details;

	// surface capabilities
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &details.capabilities);

	// surface formats
	u32 num_surface_formats = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &num_surface_formats, nullptr);

	if (num_surface_formats != 0)
	{
		details.formats = init_static_array<VkSurfaceFormatKHR>(arena, num_surface_formats);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &num_surface_formats, details.formats.data);
	}

	// present modes
	u32 num_present_modes = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &num_present_modes, nullptr);

	if (num_present_modes != 0)
	{
		details.present_modes = init_static_array<VkPresentModeKHR>(arena, num_present_modes);
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

static void print_instance_info(arena_t& arena)
{
	if (!is_debug())
	{
		return;
	}

	// print supported extensions
	{
		u32 num_exts;
		vkEnumerateInstanceExtensionProperties(nullptr, &num_exts, nullptr);

		static_array_t<VkExtensionProperties> instance_extensions = init_static_array<VkExtensionProperties>(arena, num_exts);
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

		static_array_t<VkLayerProperties> instance_layers = init_static_array<VkLayerProperties>(arena, num_layers);
		vkEnumerateInstanceLayerProperties(&num_layers, instance_layers.data);

		debug_printf("Supported Instance Validation Layers\n");
		for (u32 i = 0; i < num_layers; i++)
		{
			const VkLayerProperties& l = instance_layers[i];
			debug_printf("  Name: %s | Desc: %s | Spec Version: %i\n", l.layerName, l.description, l.specVersion);
		}
	}
}

static bool check_validation_layer_support(arena_t& arena)
{
	u32 num_layers;
	vkEnumerateInstanceLayerProperties(&num_layers, nullptr);

	static_array_t<VkLayerProperties> instance_layers = init_static_array<VkLayerProperties>(arena, num_layers);
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

static bool check_physical_device_extension_support(arena_t& arena, VkPhysicalDevice device)
{
	u32 num_extensions = 0;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &num_extensions, nullptr);

	static_array_t<VkExtensionProperties> extensions = init_static_array<VkExtensionProperties>(arena, num_extensions);
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

static bool is_physical_device_suitable(arena_t& arena, VkPhysicalDevice device, VkSurfaceKHR surface)
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
	if (swapchain_info.formats.size == 0 || swapchain_info.present_modes.size == 0)
	{
		return false;
	}

	vk_queue_indices_t queue_indices = find_queue_indices(arena, device, surface);
	return has_required_queues(queue_indices);
}


void sm::init_renderer(window_t* window)
{	
	s_window = window;

	arena_t* startup_arena = init_arena(MiB(1));

	// vk instance
	{
        load_vulkan_global_funcs();
        print_instance_info(*startup_arena);

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
            SM_ASSERT(check_validation_layer_support(*startup_arena));
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

	// vk device
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
				if(is_physical_device_suitable(*startup_arena, device, s_surface))
				{
					selected_phy_device = device;
					vkGetPhysicalDeviceProperties(selected_phy_device, &selected_phys_device_props);
					vkGetPhysicalDeviceMemoryProperties(selected_phy_device, &selected_phys_device_mem_props);
					queue_indices = find_queue_indices(*startup_arena, device, s_surface);
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
        vk_swapchain_info_t swapchain_info = query_swapchain(*startup_arena, s_phys_device, s_surface);

        VkSurfaceFormatKHR swapchain_format = swapchain_info.formats[0];
		for(int i = 0; i < swapchain_info.formats.size; i++)
        {
			const VkSurfaceFormatKHR& format = swapchain_info.formats[i];
            if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                swapchain_format = format;
            }
        }

		VkPresentModeKHR swapchain_present_mode = VK_PRESENT_MODE_FIFO_KHR;
		for(int i = 0; i < swapchain_info.present_modes.size; i++)
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

        s_swapchain_images = init_static_array<VkImage>(*startup_arena, num_images);
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
        for (u32 i = 0; i < (u32)s_swapchain_images.size; i++)
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
            // frame wender data, infinite grid, post processing
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

		//// Main draw render pass
		//{
		//    m_mainDrawRenderPass.PreInitAddAttachmentDesc(VulkanFormats::GetMainColorFormat(), VulkanDevice::Get()->m_maxNumMsaaSamples,
		//                                                  VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		//                                                  VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, 
		//                                                  VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, 0);

		//    m_mainDrawRenderPass.PreInitAddAttachmentDesc(VulkanFormats::GetMainDepthFormat(), VulkanDevice::Get()->m_maxNumMsaaSamples,
		//                                                  VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		//                                                  VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, 
		//                                                  VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, 0);

		//    m_mainDrawRenderPass.PreInitAddAttachmentDesc(VulkanFormats::GetMainColorFormat(), VK_SAMPLE_COUNT_1_BIT,
		//                                                  VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		//                                                  VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, 
		//                                                  VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, 0);

		//    m_mainDrawRenderPass.PreInitAddSubpassAttachmentReference(0, VulkanSubpass::COLOR, 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		//    m_mainDrawRenderPass.PreInitAddSubpassAttachmentReference(0, VulkanSubpass::DEPTH, 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
		//    m_mainDrawRenderPass.PreInitAddSubpassAttachmentReference(0, VulkanSubpass::COLOR_RESOLVE, 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		//    m_mainDrawRenderPass.PreInitAddSubpassDependency(VK_SUBPASS_EXTERNAL, 0,
		//                                                     VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
		//                                                     VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
		//                                                     0, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, 
		//                                                     0);

		//    m_mainDrawRenderPass.Init();
		//}

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


            VkSubpassDescription2 subpass_desc = {};
            subpass_desc.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2;
            subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass_desc.colorAttachmentCount = (U32)subpass.m_colorAttachRefs.size();
            subpass_desc.pColorAttachments = subpass.m_colorAttachRefs.data();
            subpass_desc.pResolveAttachments = subpass.m_colorResolveAttachRefs.data();
            subpass_desc.pDepthStencilAttachment = subpass.m_bHasDepthAttach ? &subpass.m_depthAttachRef : VK_NULL_HANDLE;

            // resolve multisample depth if its needed using pNext w/ struct 
            VkSubpassDescriptionDepthStencilResolve depthStencilResolve = {};
            if (subpass.m_bHasDepthResolveAttach)
            {
                depthStencilResolve.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_DEPTH_STENCIL_RESOLVE;
                depthStencilResolve.depthResolveMode = VK_RESOLVE_MODE_MAX_BIT;
                depthStencilResolve.pDepthStencilResolveAttachment = &subpass.m_depthResolveAttachRef;
                subpass_desc.pNext = &depthStencilResolve;
            }

		}

        //// Imgui
        //{
        //    m_imguiRenderPass.PreInitAddAttachmentDesc(VulkanFormats::GetMainColorFormat(), VK_SAMPLE_COUNT_1_BIT,
        //        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        //        VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE,
        //        VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
        //        0);

        //    m_imguiRenderPass.PreInitAddSubpassAttachmentReference(0, VulkanSubpass::COLOR, 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        //    m_imguiRenderPass.PreInitAddSubpassDependency(VK_SUBPASS_EXTERNAL, 0, 
        //                                                  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 
        //                                                  0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

        //    m_imguiRenderPass.Init();
        //}

	}
}