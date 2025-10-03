#pragma once

#define VK_NO_PROTOTYPES
#include "third_party/vulkan/vulkan.h"

namespace sm
{
    void vulkan_global_funcs_load();
    void vulkan_instance_funcs_load(VkInstance instance);
    void vulkan_device_funcs_load(VkDevice device);
}

#define VK_EXPORTED_FUNCTION(func)	extern PFN_##func func;
#define VK_GLOBAL_FUNCTION(func)	extern PFN_##func func;
#define VK_INSTANCE_FUNCTION(func)	extern PFN_##func func;
#define VK_DEVICE_FUNCTION(func)	extern PFN_##func func;

#include "sm/render/vulkan/vk_functions_manifest.h"
