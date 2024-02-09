#pragma once

#include "Engine/Render/Renderer.h"
#include "Engine/Config.h"
#include "Engine/Core/Types.h"
#include "Engine/Render/Vulkan/VulkanCommandPool.h"
#include "Engine/Render/Vulkan/VulkanDescriptorSets.h"
#include "Engine/Render/Vulkan/VulkanDevice.h"
#include "Engine/Render/Vulkan/VulkanFramebuffer.h"
#include "Engine/Render/Vulkan/VulkanInclude.h"
#include "Engine/Render/Vulkan/VulkanInstance.h"
#include "Engine/Render/Vulkan/VulkanRenderPass.h"
#include "Engine/Render/Vulkan/VulkanSwapchain.h"
#include <vector>

class Camera;

struct VulkanRenderFrame
{
	U32					m_swapchainImageIndex = VulkanSwapchain::kInvalidSwapchainIndex;
	VulkanSemaphore		m_swapchainImageIsReadySemaphore;
	VulkanFence			m_frameCompletedFence;
	VulkanSemaphore		m_frameCompletedSemaphore;
	VkDescriptorSet		m_frameDescriptorSet;
	VulkanBuffer		m_frameDescriptorBuffer;
	VkCommandBuffer		m_frameCommandBuffer;
	VulkanFramebuffer	m_mainDrawFramebuffer;
	VulkanTexture		m_mainDrawColorMultisampleTexture;
	VulkanTexture		m_mainDrawDepthMultisampleTexture;
	VulkanTexture		m_mainDrawColorResolveTexture;
};

class VulkanRenderer : public Renderer
{
public:
	VulkanRenderer();

	virtual void Init(Window* pWindow) final;
	virtual void Update(F32 ds) final;
	virtual void Render() final;
	virtual void Shutdown() final;
	virtual void SetCamera(const Camera* pCamera) final;

	void InitSwapchain();
	void RefreshSwapchain();

	void InitRenderFrames();
	void DestroyRenderFrames();

	void SetupNewFrame();
	void PresentFinalImage();

	Window* m_pWindow;
	VkSurfaceKHR m_surface;
	VulkanSwapchain m_swapchain;

	VulkanCommandPool m_graphicsCommandPool;

	VulkanRenderPass m_mainDrawRenderPass;

	VulkanDescriptorSetLayout m_globalDescriptorSetLayout;
	VulkanDescriptorPool m_globalDescriptorPool;
	VkDescriptorSet m_globalDescriptorSet;
	VulkanSampler m_globalLinearSampler;

	VulkanDescriptorSetLayout m_frameDescriptorSetLayout;
	VulkanDescriptorPool m_frameDescriptorPool;

	U32 m_currentFrame;

	VulkanRenderFrame m_renderFrames[MAX_NUM_FRAMES_IN_FLIGHT];

	const Camera* m_pMainCamera;

	F32 m_elapsedTimeSeconds;
	F32 m_deltaTimeSeconds;
};
