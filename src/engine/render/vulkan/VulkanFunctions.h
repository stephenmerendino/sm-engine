#pragma once

#include "Engine/Platform/WindowsInclude.h"

#define VK_NO_PROTOTYPES
#include "Engine/ThirdParty/Vulkan/vulkan.h"
#include "Engine/ThirdParty/Vulkan/vulkan_win32.h"

void LoadVulkanGlobalFuncs();
void LoadVulkanInstanceFuncs(VkInstance instance);
void LoadVulkanDeviceFuncs(VkDevice instance);

#define VK_EXPORTED_FUNCTION(func)	extern PFN_##func func;
#define VK_GLOBAL_FUNCTION(func)	extern PFN_##func func;
#define VK_INSTANCE_FUNCTION(func)	extern PFN_##func func;
#define VK_DEVICE_FUNCTION(func)	extern PFN_##func func;

#include "VulkanFunctionsManifest.inl"