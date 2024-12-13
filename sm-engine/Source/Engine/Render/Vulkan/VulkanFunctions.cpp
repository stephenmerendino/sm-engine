#include "Engine/Render/Vulkan/VulkanFunctions.h"
#include "Engine/Core/Assert_old.h"

//#define VK_EXPORTED_FUNCTION(func)	PFN_##func func = VK_NULL_HANDLE;
//#define VK_GLOBAL_FUNCTION(func)	PFN_##func func = VK_NULL_HANDLE;
//#define VK_INSTANCE_FUNCTION(func)	PFN_##func func = VK_NULL_HANDLE;
//#define VK_DEVICE_FUNCTION(func)	PFN_##func func = VK_NULL_HANDLE;
//
//#include "Engine/Render/Vulkan/VulkanFunctionsManifest.inl"

void LoadVulkanGlobalFuncs()
{
//#define VK_EXPORTED_FUNCTION(func) \
//		HMODULE vulkanLib = LoadLibrary(L"vulkan-1.dll"); \
//		SM_ASSERT(nullptr != vulkanLib); \
//		func = (PFN_##func)GetProcAddress(vulkanLib, #func); \
//		SM_ASSERT(nullptr != func);
//
//#define VK_GLOBAL_FUNCTION(func) \
//		func = (PFN_##func)vkGetInstanceProcAddr(nullptr, #func); \
//		SM_ASSERT(nullptr != func);
//
//#include "Engine/Render/Vulkan/VulkanFunctionsManifest.inl"
}

void LoadVulkanInstanceFuncs(VkInstance instance)
{
//#define VK_INSTANCE_FUNCTION(func) \
//		func = (PFN_##func)vkGetInstanceProcAddr(instance, #func); \
//		SM_ASSERT(nullptr != func);
//
//#include "Engine/Render/Vulkan/VulkanFunctionsManifest.inl"
}

void LoadVulkanDeviceFuncs(VkDevice device)
{
//#define VK_DEVICE_FUNCTION(func) \
//		func = (PFN_##func)vkGetDeviceProcAddr(device, #func); \
//		SM_ASSERT(nullptr != func);
//
//#include "Engine/Render/Vulkan/VulkanFunctionsManifest.inl"
}