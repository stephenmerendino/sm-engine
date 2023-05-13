#pragma once

#include "engine/platform/windows_include.h"

#define VK_NO_PROTOTYPES
#include "engine/thirdparty/vulkan/vulkan.h"
#include "engine/thirdparty/vulkan/vulkan_win32.h"

void load_vulkan_global_funcs();
void load_vulkan_instance_funcs(VkInstance instance);
void load_vulkan_device_funcs(VkDevice instance);

#define VK_EXPORTED_FUNCTION(func)	extern PFN_##func func;
#define VK_GLOBAL_FUNCTION(func)	extern PFN_##func func;
#define VK_INSTANCE_FUNCTION(func)	extern PFN_##func func;
#define VK_DEVICE_FUNCTION(func)	extern PFN_##func func;

#include "vulkan_functions_manifest.inl"
