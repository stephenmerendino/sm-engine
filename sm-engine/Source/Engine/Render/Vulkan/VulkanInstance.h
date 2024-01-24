#pragma once

#include "Engine/Render/Vulkan/VulkanInclude.h"

class VulkanInstance
{
public:
	VulkanInstance();

	void Init();
	void Destroy();

	VkInstance m_instance;
	VkDebugUtilsMessengerEXT m_debugMessenger;
};
