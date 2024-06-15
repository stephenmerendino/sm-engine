#pragma once

#include "Engine/Render/Vulkan/VulkanInclude.h"
#include "Engine/Core/Types.h"

class VulkanQueueFamilies
{
public:
	VulkanQueueFamilies();

	void Init(VkPhysicalDevice device, VkSurfaceKHR surface);
	bool HasAllFamilies();
	bool HasRequiredFamilies();
	bool HasAsyncComputeFamily();

	I32 m_graphicsAndComputeFamilyIndex;
	I32 m_asyncComputeFamilyIndex;
	I32 m_presentationFamilyIndex;

	enum
	{
		kInvalidFamilyIndex = -1
	};
};
