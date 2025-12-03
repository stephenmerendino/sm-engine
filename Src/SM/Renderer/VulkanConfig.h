#pragma once

#define VK_NO_PROTOTYPES
#include "ThirdParty/vulkan/vulkan.h"

#if defined(NDEBUG)
	static const char* INSTANCE_EXTENSIONS[] = {
		"VK_KHR_surface",
		"VK_KHR_win32_surface"
	};

	static const char* DEVICE_EXTENSIONS[] = {
		"VK_KHR_swapchain",
        "VK_KHR_dynamic_rendering"
	};

	static const bool VULKAN_VERBOSE = false;
	static const bool ENABLE_VALIDATION_LAYERS = false;
	static const bool ENABLE_VALIDATION_BEST_PRACTICES = false;
#else
	static const char* INSTANCE_EXTENSIONS[] = {
		"VK_KHR_surface",
		"VK_KHR_win32_surface",
		"VK_EXT_debug_utils",
		"VK_EXT_validation_features",
	};

	static const char* DEVICE_EXTENSIONS[] = {
		"VK_KHR_swapchain",
        "VK_KHR_dynamic_rendering"
	};

	static const bool VULKAN_VERBOSE = true;
	static const bool ENABLE_VALIDATION_LAYERS = true;
	static const bool ENABLE_VALIDATION_BEST_PRACTICES = false;
#endif

static const char* VALIDATION_LAYERS[] = {
	"VK_LAYER_KHRONOS_validation"
};
