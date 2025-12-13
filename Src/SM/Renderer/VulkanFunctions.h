#pragma once

#define VK_NO_PROTOTYPES
#include "ThirdParty/vulkan/vulkan.h"

#define VK_EXPORTED_FUNCTION(func)	extern PFN_##func func;
#define VK_GLOBAL_FUNCTION(func)	extern PFN_##func func;
#define VK_INSTANCE_FUNCTION(func)	extern PFN_##func func;
#define VK_DEVICE_FUNCTION(func)	extern PFN_##func func;
#include "SM/Renderer/VulkanFunctionsManifest.inl"

