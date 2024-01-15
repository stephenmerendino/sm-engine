#pragma once

#include "Engine/Core/Types.h"
#include <vector>
#include <string>

static const U32 WINDOW_WIDTH = 1920;
static const U32 WINDOW_HEIGHT = 1080;
static const bool WINDOW_ALLOW_RESIZE = true;

static const U32 MAX_NUM_FRAMES_IN_FLIGHT = 2;

static const F32 FPS_CALC_TIME_INTERVAL_SECONDS = 0.1f;
static const F32 TARGET_FPS = 200.0f;

static const std::string MODEL_PATH = "Models/viking_room.obj";
static const std::string TEXTURE_PATH = "Textures/viking_room.png";

#if defined(NDEBUG)
	static const std::vector<const char*> INSTANCE_EXTENSIONS = {
		"VK_KHR_surface",
		"VK_KHR_win32_surface"
	};

	static const std::vector<const char*> DEVICE_EXTENSIONS = {
		"VK_KHR_swapchain"
	};

	static const bool VULKAN_VERBOSE = false;
	static const bool ENABLE_VALIDATION_LAYERS = false;
	static const bool ENABLE_VALIDATION_BEST_PRACTICES = false;
#else
	static const std::vector<const char*> INSTANCE_EXTENSIONS = {
		"VK_KHR_surface",
		"VK_KHR_win32_surface",
		"VK_EXT_debug_utils",
		"VK_EXT_validation_features"
	};

	static const std::vector<const char*> DEVICE_EXTENSIONS = {
		"VK_KHR_swapchain",
	};

	static const bool VULKAN_VERBOSE = true;
	static const bool ENABLE_VALIDATION_LAYERS = true;
	static const bool ENABLE_VALIDATION_BEST_PRACTICES = false;
#endif

static const std::vector<const char*> VALIDATION_LAYERS = {
	"VK_LAYER_KHRONOS_validation"
};
