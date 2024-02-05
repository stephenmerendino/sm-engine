#pragma once

#include "Engine/Core/Types.h"
#include "Engine/Render/Vulkan/VulkanCommandPool.h"
#include "Engine/Render/Vulkan/VulkanInclude.h"
#include "Engine/Render/Vulkan/VulkanDevice.h"

class VulkanBuffer
{
public:
	enum class Type : U8
	{
		kVertexBuffer,
		kIndexBuffer,
		kUniformBuffer,
		kStagingBuffer,
		kNumBufferTypes,
		kInvalid = 0xFF,
	};

	VulkanBuffer();

	void Init(const VulkanDevice* device, Type type, VkDeviceSize size);
	void Update(const VulkanCommandPool& commandPool, const void* data, VkDeviceSize gpuMemoryOffset);
	void Destroy();

	const VulkanDevice* m_pDevice;
	VkBuffer m_bufferHandle;
	VkDeviceMemory m_deviceMemory;
	Type m_type;
	VkDeviceSize m_size;
};
