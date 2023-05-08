#pragma once

#include "engine/render/vulkan/VulkanInclude.h"

struct vulkan_instance_t
{
    VkInstance m_instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
};

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
