#include "sm/render/vk_renderer.h"
#include "sm/render/window.h"
#include "sm/render/vk_include.h"
#include "sm/core/debug.h"
#include "sm/core/helpers.h"
#include "sm/config.h"

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
VkPhysicalDeviceProperties s_phys_device_props = {};
VkPhysicalDeviceMemoryProperties s_phys_device_mem_props = {};
VkDevice s_device = VK_NULL_HANDLE;
vk_queue_indices_t s_queue_indices;
VkQueue s_graphics_queue = VK_NULL_HANDLE;
VkQueue s_async_compute_queue = VK_NULL_HANDLE;
VkQueue s_present_queue = VK_NULL_HANDLE;
VkSampleCountFlagBits s_max_msaa_samples = VK_SAMPLE_COUNT_1_BIT;
VkCommandPool s_graphics_command_pool;
VkFormat s_main_color_format = VK_FORMAT_R8G8B8A8_UNORM;
VkFormat s_depth_format = VK_FORMAT_UNDEFINED;

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
}