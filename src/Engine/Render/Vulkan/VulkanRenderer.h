#pragma once

class Camera;
class RenderableMesh;
class Window;
class VulkanBuffer;
class VulkanCommandPool;
class VulkanDescriptorPool;
class VulkanDescriptorSetLayout;
class VulkanDevice;
class VulkanPipeline;
class VulkanRenderPass;
class VulkanSampler;
class VulkanSurface;
class VulkanSwapchain;
class VulkanTexture;

#include "Engine/Core/Types.h"
#include "Engine/Render/Vulkan/VulkanInclude.h"
#include <vector>

class VulkanRenderer
{
public:
	VulkanRenderer(Window* pWindow);
	~VulkanRenderer();

	bool Setup();
	void Teardown();
	void TeardownSwapchain();
	void ResetupSwapChain();

	void SetupMeshes();
	void SetupTexturesAndSamplers();
	void SetupUniformBuffers();
	void SetupRenderPass();
	void SetupFrameBuffers();
	void SetupDescriptorSets();
	void SetupPipelines();
	void SetupCommandBuffers();
	void SetupSyncObjects();
	void SetCamera(Camera* pCamera);

	void RenderFrame();
	void UpdateUniformBuffer(U32 currentImage);

private:
	Window* m_pWindow;
	VulkanSurface* m_pSurface;
	VulkanDevice* m_pDevice;
	VulkanSwapchain* m_pSwapchain;
	std::vector<VkFramebuffer> m_vkSwapChainFrameBuffers;
	VulkanCommandPool* m_pGraphicsCommandPool;

	std::vector<VulkanBuffer*> m_pUniformBuffers;
	VulkanRenderPass* m_pRenderPass;

	VulkanTexture* m_pColorTarget;
	VulkanTexture* m_pDepthTarget;
	VulkanSampler* m_pLinearSampler;

	VulkanTexture* m_pVikingRoomTexture;
	RenderableMesh* m_pVikingRoomMesh;
	VulkanPipeline* m_pVikingRoomPipeline;
	RenderableMesh* m_pWorldAxesMesh;
	VulkanPipeline* m_pWorldAxesPipeline;

	VulkanDescriptorSetLayout* m_pDescriptorSetLayout;
	VulkanDescriptorPool* m_pDescriptorPool;
	std::vector<VkDescriptorSet> m_vkDescriptorSets;
	
	std::vector<VkCommandBuffer> m_vkCommandBuffers;

	std::vector<VkSemaphore> m_vkImageAvailableSemaphores;
	std::vector<VkSemaphore> m_vkRenderFinishedSemaphores;
	std::vector<VkFence> m_vkFrameFinishedFences;
	std::vector<VkFence> m_vkSwapChainImagesInFlight;

	U32 m_currentFrame;
	Camera* m_pCurrentCamera;
};