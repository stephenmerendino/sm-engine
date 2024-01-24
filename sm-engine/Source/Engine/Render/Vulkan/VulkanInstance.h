#pragma once

#include "Engine/Render/Vulkan/VulkanInclude.h"

class VulkanInstance
{
public:
	VulkanInstance();

	void Init();
	void Destroy();

	VkInstance m_instanceHandle;
	VkDebugUtilsMessengerEXT m_debugMessengerHandle;
};
