#pragma once

#define VK_NO_PROTOTYPES
#include "Engine/ThirdParty/vulkan/vulkan.h"

#include "Engine/Render/Vulkan/VulkanFunctions.h"

#include "Engine/Core/Assert.h"

#if ASSERTIONS_ENABLED
#define SM_VULKAN_ASSERT(vkResult) SM_ASSERT_MSG(VK_SUCCESS == vkResult, "Vulkan Assertion Failed")
#else
#define SM_VULKAN_ASSERT(vkResult) vkResult
#endif