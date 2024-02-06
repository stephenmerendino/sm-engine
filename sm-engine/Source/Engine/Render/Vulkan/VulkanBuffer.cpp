#include "Engine/Render/Vulkan/VulkanBuffer.h"
#include "Engine/Render/Vulkan/VulkanCommands.h"

static void InteralInit(VulkanBuffer* bufferToInit, VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags)
{
	VkBufferCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	createInfo.size = size;
	createInfo.usage = usageFlags;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	SM_VULKAN_ASSERT(vkCreateBuffer(VulkanDevice::GetHandle(), &createInfo, nullptr, &bufferToInit->m_bufferHandle));

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(VulkanDevice::GetHandle(), bufferToInit->m_bufferHandle, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = VulkanDevice::Get()->FindSupportedMemoryType(memRequirements.memoryTypeBits, memoryPropertyFlags);

	SM_VULKAN_ASSERT(vkAllocateMemory(VulkanDevice::GetHandle(), &allocInfo, nullptr, &bufferToInit->m_deviceMemory));

	vkBindBufferMemory(VulkanDevice::GetHandle(), bufferToInit->m_bufferHandle, bufferToInit->m_deviceMemory, 0);
}

VulkanBuffer::VulkanBuffer()
	:m_bufferHandle(VK_NULL_HANDLE)
	,m_deviceMemory(VK_NULL_HANDLE)
	,m_type(Type::kInvalid)
	,m_size(0)
{
}

void VulkanBuffer::Init(VulkanBuffer::Type type, VkDeviceSize size)
{
	m_type = type;
	m_size = size;

	switch (type)
	{
		case Type::kVertexBuffer:   InteralInit(this, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT); break;
		case Type::kIndexBuffer:    InteralInit(this, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT); break;
		case Type::kUniformBuffer:  InteralInit(this, size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT); break;
		case Type::kStagingBuffer:  InteralInit(this, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT); break;

		case Type::kNumBufferTypes:
		case Type::kInvalid:
		default: SM_ERROR_MSG("Tried to init VulkanBuffer with incorrect type\n");
	}
}

void VulkanBuffer::Update(const VulkanCommandPool& commandPool, const void* data, VkDeviceSize gpuMemoryOffset)
{
	if (m_type == Type::kUniformBuffer || m_type == Type::kStagingBuffer)
	{
		void* mappedGpuMem;
		vkMapMemory(VulkanDevice::Get()->m_deviceHandle, m_deviceMemory, gpuMemoryOffset, m_size, 0, &mappedGpuMem);
		memcpy(mappedGpuMem, data, (size_t)m_size);
		vkUnmapMemory(VulkanDevice::Get()->m_deviceHandle, m_deviceMemory);
	}
	else if (m_type == Type::kVertexBuffer || m_type == Type::kIndexBuffer)
	{
		VulkanBuffer stagingBuffer;
		stagingBuffer.Init(Type::kStagingBuffer, m_size);
		stagingBuffer.Update(commandPool, data, gpuMemoryOffset);

		VkCommandBuffer updateCommand = commandPool.BeginSingleTime();
		VulkanCommands::CopyBuffer(updateCommand, stagingBuffer, *this, m_size);
		commandPool.EndAndSubmitSingleTime(updateCommand);

		stagingBuffer.Destroy();
	}
}

void VulkanBuffer::Destroy()
{
	vkDestroyBuffer(VulkanDevice::GetHandle(), m_bufferHandle, nullptr);
	vkFreeMemory(VulkanDevice::GetHandle(), m_deviceMemory, nullptr);
}