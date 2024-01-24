#include "Engine/Render/Vulkan/VulkanQueueFamilies.h"
#include <vector>

VulkanQueueFamilies::VulkanQueueFamilies()
	:m_graphicsFamilyIndex(kInvalidFamilyIndex)
	,m_presentFamilyIndex(kInvalidFamilyIndex)
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
			m_graphicsFamilyIndex = i;
		}

		// query for presentation support on this queue
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
		if (presentSupport)
		{
			m_presentFamilyIndex = i;
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
	return m_graphicsFamilyIndex != kInvalidFamilyIndex && m_presentFamilyIndex != kInvalidFamilyIndex;
}
