#pragma once

#include "sm/core/color.h"
#include "sm/core/types.h"
#include <vector>
#include <string>

static const sm::u32 WINDOW_WIDTH = 1920;
static const sm::u32 WINDOW_HEIGHT = 1080;
static const bool WINDOW_ALLOW_RESIZE = true;

static const sm::u32 MAX_NUM_FRAMES_IN_FLIGHT = 2;

static const sm::f32 FPS_CALC_TIME_INTERVAL_SECONDS = 0.1f;
static const sm::f32 MAX_FPS = 240.0f;

static const char* SHADERS_PATH = "built_assets/shaders/";
static const char* TEXTURES_PATH = "built_assets/textures/";
static const char* MODELS_PATH = "built_assets/models/";

static const sm::color_f32_t CLEAR_COLOR { .r = 0.20f, .g = 0.20f, .b = 0.20f, .a = 1.0f };

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
