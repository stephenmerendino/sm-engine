#include "Engine/Render/Vulkan/VulkanQueueFamilies.h"
#include <vector>

VulkanQueueFamilies::VulkanQueueFamilies()
	:m_graphicsAndComputeFamilyIndex(kInvalidFamilyIndex)
	,m_presentationFamilyIndex(kInvalidFamilyIndex)
	,m_asyncComputeFamilyIndex(kInvalidFamilyIndex)
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
		if (m_graphicsAndComputeFamilyIndex == kInvalidFamilyIndex && 
			(props.queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) == (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT))
		{
			m_graphicsAndComputeFamilyIndex = i;
		}

		if (m_asyncComputeFamilyIndex == kInvalidFamilyIndex && 
			(props.queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) == VK_QUEUE_COMPUTE_BIT)
		{
			m_asyncComputeFamilyIndex = i;
		}

		// query for presentation support on this queue
		VkBool32 isPresentationSupported = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &isPresentationSupported);
		if (m_presentationFamilyIndex == kInvalidFamilyIndex && isPresentationSupported)
		{
			m_presentationFamilyIndex = i;
		}

		// has required families
		if (HasAllFamilies())
		{
			break;
		}
	}
}

bool VulkanQueueFamilies::HasAllFamilies()
{
	return m_graphicsAndComputeFamilyIndex != kInvalidFamilyIndex && 
		   m_presentationFamilyIndex != kInvalidFamilyIndex &&
		   m_asyncComputeFamilyIndex != kInvalidFamilyIndex;
}

bool VulkanQueueFamilies::HasRequiredFamilies()
{
	return m_graphicsAndComputeFamilyIndex != kInvalidFamilyIndex && m_presentationFamilyIndex != kInvalidFamilyIndex;
}

bool VulkanQueueFamilies::HasAsyncComputeFamily()
{
	return m_asyncComputeFamilyIndex != kInvalidFamilyIndex;
}
