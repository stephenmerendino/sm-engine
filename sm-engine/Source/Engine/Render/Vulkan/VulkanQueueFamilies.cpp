#include "Engine/Render/Vulkan/VulkanQueueFamilies.h"
#include <vector>

VulkanQueueFamilies::VulkanQueueFamilies()
	:m_graphicsFamily(kInvalidFamilyIndex)
	,m_presentFamily(kInvalidFamilyIndex)
{
}

void VulkanQueueFamilies::Init(VkPhysicalDevice device, VkSurfaceKHR surface)
{
	U32 queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilyProps(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyProps.data());

	for (int i = 0; i < (int)queueFamilyProps.size(); i++)
	{
		const VkQueueFamilyProperties& props = queueFamilyProps[i];
		if (props.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			m_graphicsFamily = i;
		}

		// query for presentation support on this queue
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
		if (presentSupport)
		{
			m_presentFamily = i;
		}

		// has required families
		if (HasRequiredFamilies())
		{
			break;
		}
	}
}

bool VulkanQueueFamilies::HasRequiredFamilies()
{
	return m_graphicsFamily != kInvalidFamilyIndex && m_presentFamily != kInvalidFamilyIndex;
}
