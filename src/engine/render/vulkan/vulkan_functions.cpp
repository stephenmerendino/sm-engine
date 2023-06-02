#include "engine/render/vulkan/vulkan_functions.h"
#include "engine/core/assert.h"

#define VK_EXPORTED_FUNCTION(func)	PFN_##func func = VK_NULL_HANDLE;
#define VK_GLOBAL_FUNCTION(func)	PFN_##func func = VK_NULL_HANDLE;
#define VK_INSTANCE_FUNCTION(func)	PFN_##func func = VK_NULL_HANDLE;
#define VK_DEVICE_FUNCTION(func)	PFN_##func func = VK_NULL_HANDLE;

#include "vulkan_functions_manifest.inl"

void load_vulkan_global_funcs()
{
	#define VK_EXPORTED_FUNCTION(func) \
		HMODULE vulkanLib = LoadLibrary("vulkan-1.dll"); \
		SM_ASSERT(nullptr != vulkanLib); \
		func = (PFN_##func)GetProcAddress(vulkanLib, #func); \
		SM_ASSERT(nullptr != func);

	#define VK_GLOBAL_FUNCTION(func) \
		func = (PFN_##func)vkGetInstanceProcAddr(nullptr, #func); \
		SM_ASSERT(nullptr != func);

	#include "vulkan_functions_manifest.inl"
}

void load_vulkan_instance_funcs(VkInstance instance)
{
	#define VK_INSTANCE_FUNCTION(func) \
		func = (PFN_##func)vkGetInstanceProcAddr(instance, #func); \
		SM_ASSERT(nullptr != func);

	#include "vulkan_functions_manifest.inl"
}

void load_vulkan_device_funcs(VkDevice device)
{
	#define VK_DEVICE_FUNCTION(func) \
		func = (PFN_##func)vkGetDeviceProcAddr(device, #func); \
		SM_ASSERT(nullptr != func);

	#include "vulkan_functions_manifest.inl"
}
