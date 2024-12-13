#pragma once

#define VK_NO_PROTOTYPES
#include "third_party/vulkan/vulkan.h"

#include "sm/render/vk_functions.h"
#include "sm/core/assert.h"

#if ASSERTIONS_ENABLED
#define SM_VULKAN_ASSERT(vk_result) SM_ASSERT_MSG(VK_SUCCESS == vk_result, "Vulkan Assertion Failed")
#else
#define SM_VULKAN_ASSERT(vk_result) vkResult
#endif
