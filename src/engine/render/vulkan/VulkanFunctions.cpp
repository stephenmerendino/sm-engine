#include "engine/render/vulkan/VulkanFunctions.h"
#include "engine/core/Assert.h"

#define VK_EXPORTED_FUNCTION(func)	PFN_##func func = VK_NULL_HANDLE;
#define VK_GLOBAL_FUNCTION(func)	PFN_##func func = VK_NULL_HANDLE;
#define VK_INSTANCE_FUNCTION(func)	PFN_##func func = VK_NULL_HANDLE;
#define VK_DEVICE_FUNCTION(func)	PFN_##func func = VK_NULL_HANDLE;

#include "VulkanFunctionsManifest.inl"

void LoadVulkanGlobalFuncs()
{
	#define VK_EXPORTED_FUNCTION(func) \
		HMODULE vulkanLib = LoadLibrary("vulkan-1.dll"); \
		ASSERT(nullptr != vulkanLib); \
		func = (PFN_##func)GetProcAddress(vulkanLib, #func); \
		ASSERT(nullptr != func);

	#define VK_GLOBAL_FUNCTION(func) \
		func = (PFN_##func)vkGetInstanceProcAddr(nullptr, #func); \
		ASSERT(nullptr != func);

	#include "VulkanFunctionsManifest.inl"
}

void LoadVulkanInstanceFuncs(VkInstance instance)
{
	#define VK_INSTANCE_FUNCTION(func) \
		func = (PFN_##func)vkGetInstanceProcAddr(instance, #func); \
		ASSERT(nullptr != func);

	#include "VulkanFunctionsManifest.inl"
}

void LoadVulkanDeviceFuncs(VkDevice device)
{
	#define VK_DEVICE_FUNCTION(func) \
		func = (PFN_##func)vkGetDeviceProcAddr(device, #func); \
		ASSERT(nullptr != func);

	#include "VulkanFunctionsManifest.inl"
}
