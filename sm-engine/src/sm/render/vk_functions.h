#pragma once

#define VK_NO_PROTOTYPES
#include "third_party/vulkan/vulkan.h"

#if defined _WIN32
#include "sm/platform/win32/win32_include.h"
#include "third_party/vulkan/vulkan_win32.h"
#endif

namespace sm
{
    void load_vulkan_global_funcs();
    void load_vulkan_instance_funcs(VkInstance instance);
    void load_vulkan_device_funcs(VkDevice device);
}

#define VK_EXPORTED_FUNCTION(func)	extern PFN_##func func;
#define VK_GLOBAL_FUNCTION(func)	extern PFN_##func func;
#define VK_INSTANCE_FUNCTION(func)	extern PFN_##func func;
#define VK_DEVICE_FUNCTION(func)	extern PFN_##func func;

#include "sm/render/vk_functions_manifest.inl"

#if defined _WIN32
#include "sm/platform/win32/render/win32_vk_functions_manifest.inl"
#endif
