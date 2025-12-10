#pragma once

#include "SM/StandardTypes.h"

namespace SM
{
    namespace VulkanConfig
    {
        #if defined(NDEBUG)
        static const char* kInstanceExtensions[] = {
            "VK_KHR_surface",
            "VK_KHR_win32_surface"
        };

        static const char* kDeviceExtensions[] = {
            "VK_KHR_swapchain",
            "VK_KHR_dynamic_rendering"
        };

        static const bool kEnableVerboseLog = false;
        static const bool kEnableValidationLayers = false;
        static const bool kEnableValidationBestPractices = false;
        #else
        static const char* kInstanceExtensions[] = {
            "VK_KHR_surface",
            "VK_KHR_win32_surface",
            "VK_EXT_debug_utils",
            "VK_EXT_validation_features",
        };

        static const char* kDeviceExtensions[] = {
            "VK_KHR_swapchain",
            "VK_KHR_dynamic_rendering"
        };

        static const bool kEnableVerboseLog = true;
        static const bool kEnableValidationLayers = true;
        static const bool kEnableValidationBestPractices = false;
        #endif

        static const char* kValidationLayers[] = {
            "VK_LAYER_KHRONOS_validation"
        };

        static const U32 kOptimalNumFramesInFlight = 3;
    }
}
