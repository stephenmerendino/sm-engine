#pragma once

#define VK_NO_PROTOTYPES
#include "third_party/vulkan/vulkan.h"

#include "Engine/Render/Vulkan/VulkanFunctions.h"

#include "Engine/Core/Assert_old.h"

#if ASSERTIONS_ENABLED
#define SM_VULKAN_ASSERT_OLD(vkResult) SM_ASSERT_MSG_OLD(VK_SUCCESS == vkResult, "Vulkan Assertion Failed")
#else
#define SM_VULKAN_ASSERT_OLD(vkResult) vkResult
#endif