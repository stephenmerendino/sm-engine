#include "sm/render/vk_functions.h"
#include "sm/core/assert.h"

#define VK_EXPORTED_FUNCTION(func)	PFN_##func func = VK_NULL_HANDLE;
#define VK_GLOBAL_FUNCTION(func)	PFN_##func func = VK_NULL_HANDLE;
#define VK_INSTANCE_FUNCTION(func)	PFN_##func func = VK_NULL_HANDLE;
#define VK_DEVICE_FUNCTION(func)	PFN_##func func = VK_NULL_HANDLE;
#include "sm/render/vk_functions_manifest.h"

using namespace sm;

void sm::vulkan_global_funcs_load()
{
#define VK_EXPORTED_FUNCTION(func) \
		HMODULE vulkan_lib = LoadLibrary(L"vulkan-1.dll"); \
		SM_ASSERT(nullptr != vulkan_lib); \
		func = (PFN_##func)GetProcAddress(vulkan_lib, #func); \
		SM_ASSERT(nullptr != func);

#define VK_GLOBAL_FUNCTION(func) \
		func = (PFN_##func)vkGetInstanceProcAddr(nullptr, #func); \
		SM_ASSERT(nullptr != func);

#include "sm/render/vk_functions_manifest.h"
}

void sm::vulkan_instance_funcs_load(VkInstance instance)
{
#define VK_INSTANCE_FUNCTION(func) \
		func = (PFN_##func)vkGetInstanceProcAddr(instance, #func); \
		SM_ASSERT(nullptr != func);

#include "sm/render/vk_functions_manifest.h"
}

void sm::vulkan_device_funcs_load(VkDevice device)
{
#define VK_DEVICE_FUNCTION(func) \
		func = (PFN_##func)vkGetDeviceProcAddr(device, #func); \
		SM_ASSERT(nullptr != func);

#include "sm/render/vk_functions_manifest.h"
}
