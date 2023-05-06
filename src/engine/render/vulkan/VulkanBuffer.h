#pragma once

#include "Engine/Render/Vulkan/VulkanInclude.h"
#include "Engine/Core/Types.h"

class VulkanDevice;
class VulkanCommandPool;

enum class VulkanBufferType : U8
{
	kVertexBuffer,
	kIndexBuffer,
	kUniformBuffer,
	kNumBufferTypes,
	kInvalidBuffer = 0xFF,
};

class VulkanBuffer
{
public:
	VulkanBuffer(VulkanDevice* pDevice);
	~VulkanBuffer();

	bool SetupBuffer(VulkanBufferType type, size_t size);
	void Teardown();

	VkBuffer GetHandle();
	VulkanBufferType GetType() const;
	size_t GetSize() const;

	void Update(VulkanCommandPool* pGraphicsCommandPool, void* data);

private:
	VulkanDevice* m_pDevice;
	VkBuffer m_vkBuffer;
	VkDeviceMemory m_vkBufferMemory;
	VulkanBufferType m_bufferType;
	size_t m_bufferSize;
};