#include "Engine/Render/Vulkan/VulkanBuffer.h"
#include "Engine/Render/Vulkan/VulkanDevice.h"
#include "Engine/Render/Vulkan/VulkanCommandPool.h"
#include "Engine/Render/Vulkan/VulkanCommands.h"
#include "Engine/Core/Assert.h"

VulkanBuffer::VulkanBuffer(VulkanDevice* pDevice)
	:m_pDevice(pDevice)
	,m_vkBuffer(VK_NULL_HANDLE)
	,m_vkBufferMemory(VK_NULL_HANDLE)
	,m_bufferSize(0)
	,m_bufferType(VulkanBufferType::kInvalidBuffer)
{
}

VulkanBuffer::~VulkanBuffer()
{
}

bool VulkanBuffer::SetupBuffer(VulkanBufferType type, size_t size)
{
	m_bufferType = type;
	m_bufferSize = size;

	switch (m_bufferType)
	{
		case VulkanBufferType::kVertexBuffer:	m_pDevice->CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_vkBuffer, m_vkBufferMemory); break;
		case VulkanBufferType::kIndexBuffer:	m_pDevice->CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_vkBuffer, m_vkBufferMemory); break;
		case VulkanBufferType::kUniformBuffer:	m_pDevice->CreateBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_vkBuffer, m_vkBufferMemory); break;
	}

	return true;
}

void VulkanBuffer::Teardown()
{
	vkFreeMemory(m_pDevice->GetHandle(), m_vkBufferMemory, nullptr);
	vkDestroyBuffer(m_pDevice->GetHandle(), m_vkBuffer, nullptr);
}

VkBuffer VulkanBuffer::GetHandle()
{
	return m_vkBuffer;
}

VulkanBufferType VulkanBuffer::GetType() const
{
	return m_bufferType;
}

size_t VulkanBuffer::GetSize() const
{
	return m_bufferSize;
}

void VulkanBuffer::Update(VulkanCommandPool* pGraphicsCommandPool, void* data)
{
	if (m_bufferType == VulkanBufferType::kUniformBuffer)
	{
		m_pDevice->CopyIntoMemory(data, m_bufferSize, m_vkBufferMemory);
	}
	else if (m_bufferType == VulkanBufferType::kVertexBuffer || m_bufferType == VulkanBufferType::kIndexBuffer)
	{
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		m_pDevice->CreateBuffer(m_bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		m_pDevice->CopyIntoMemory(data, m_bufferSize, stagingBufferMemory);
		VulkanCommands::CopyBuffer(pGraphicsCommandPool, stagingBuffer, m_vkBuffer, m_bufferSize);

		vkDestroyBuffer(m_pDevice->GetHandle(), stagingBuffer, nullptr);
		vkFreeMemory(m_pDevice->GetHandle(), stagingBufferMemory, nullptr);
	}
}
