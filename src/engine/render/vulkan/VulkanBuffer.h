#pragma once

#include "engine/render/vulkan/VulkanInclude.h"
#include "engine/core/Types.h"

class VulkanDevice;
class VulkanCommandPool;

enum class BufferType : u8
{
	kVertexBuffer,
	kIndexBuffer,
	kUniformBuffer,
	kNumBufferTypes,
	kInvalidBuffer = 0xFF,
};

struct buffer_t
{
    VkBuffer m_vk_handle = VK_NULL_HANDLE;   
    VkDeviceMemory m_memory;
    size_t m_size;
    BufferType m_type;
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
