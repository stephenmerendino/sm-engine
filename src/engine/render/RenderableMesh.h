#pragma once

#include "Engine/Render/Vulkan/VulkanInclude.h"
#include "Engine/Core/Types.h"

#include <vector>

class Mesh;
class VulkanDevice;
class VulkanCommandPool;
class VulkanBuffer;

class RenderableMesh
{
public:
	RenderableMesh(VulkanDevice* pDevice, Mesh* pMesh);
	~RenderableMesh();

	bool Setup(VulkanCommandPool* pGraphicsCommandPool);
	void Teardown();

	VkBuffer GetVertexBuffer();
	VkBuffer GetIndexBuffer();
	U32 GetNumVertices() const;
	U32 GetNumIndices() const;
	std::vector<VkVertexInputBindingDescription> GetVertexInputBindingDesc() const;
	std::vector<VkVertexInputAttributeDescription> GetVertexAttrDescs() const;

private:
	VulkanDevice* m_pDevice;
	Mesh* m_pMesh;
	VulkanBuffer* m_pVertexBuffer;
	VulkanBuffer* m_pIndexBuffer;
};
