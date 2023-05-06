#include "engine/render/RenderableMesh.h"
#include "engine/render/Mesh.h"
#include "engine/render/vulkan/VulkanDevice.h"
#include "engine/render/vulkan/VulkanCommandPool.h"
#include "engine/render/vulkan/VulkanBuffer.h"

RenderableMesh::RenderableMesh(VulkanDevice* pDevice, Mesh* pMesh)
	:m_pDevice(pDevice)
	,m_pMesh(pMesh)
	,m_pVertexBuffer(nullptr)
	,m_pIndexBuffer(nullptr)
{
}

RenderableMesh::~RenderableMesh()
{
}

bool RenderableMesh::Setup(VulkanCommandPool* pGraphicsCommandPool)
{
	m_pVertexBuffer = new VulkanBuffer(m_pDevice);
	m_pVertexBuffer->SetupBuffer(VulkanBufferType::kVertexBuffer, m_pMesh->CalculateVertexBufferSize());
	m_pVertexBuffer->Update(pGraphicsCommandPool, (void*)m_pMesh->GetVertices().data());

	m_pIndexBuffer = new VulkanBuffer(m_pDevice);
	m_pIndexBuffer->SetupBuffer(VulkanBufferType::kIndexBuffer, m_pMesh->CalculateIndexBufferSize());
	m_pIndexBuffer->Update(pGraphicsCommandPool, (void*)m_pMesh->GetIndices().data());

	return true;
}

void RenderableMesh::Teardown()
{
	m_pVertexBuffer->Teardown();
	delete m_pVertexBuffer;

	m_pIndexBuffer->Teardown();
	delete m_pIndexBuffer;
}

VkBuffer RenderableMesh::GetVertexBuffer()
{
	return m_pVertexBuffer->GetHandle();
}

VkBuffer RenderableMesh::GetIndexBuffer()
{
	return m_pIndexBuffer->GetHandle();
}

U32 RenderableMesh::GetNumVertices() const
{
	return m_pMesh->GetNumVertices();
}

U32 RenderableMesh::GetNumIndices() const
{
	return m_pMesh->GetNumIndices();
}

std::vector<VkVertexInputBindingDescription> RenderableMesh::GetVertexInputBindingDesc() const
{
	return m_pMesh->GetVertexInputBindingDesc();
}

std::vector<VkVertexInputAttributeDescription> RenderableMesh::GetVertexAttrDescs() const
{
	return m_pMesh->GetVertexAttrDescs();
}
