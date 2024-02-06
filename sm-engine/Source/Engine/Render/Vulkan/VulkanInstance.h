#pragma once

#include "Engine/Render/Vulkan/VulkanInclude.h"

class VulkanInstance
{
public:
	static void Init();
	static void Destroy();
	static VulkanInstance* Get();
	static VkInstance GetHandle();

	VkInstance m_instanceHandle;
	VkDebugUtilsMessengerEXT m_debugMessengerHandle;

private:
	VulkanInstance();

	static VulkanInstance* s_instance;
};
