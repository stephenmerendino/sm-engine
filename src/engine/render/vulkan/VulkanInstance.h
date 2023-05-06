#pragma once

#include "Engine/Render/Vulkan/VulkanInclude.h"

class VulkanInstance
{
public:
	static bool Setup();
	static void Teardown();
	static VkInstance GetHandle();

private:
	VulkanInstance() = delete;
	~VulkanInstance() = delete;

	static VkInstance m_vkInstance;
	static VkDebugUtilsMessengerEXT m_debugMessenger;
};