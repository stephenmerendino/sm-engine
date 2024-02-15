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
#include "Engine/Render/Vulkan/VulkanPipeline.h"
#include "Engine/Render/Vulkan/VulkanRenderPass.h"
#include "Engine/Render/Vulkan/VulkanSwapchain.h"
#include <vector>

class Camera;

struct VulkanRenderFrame
{
	// Swapchain
	U32					m_swapchainImageIndex = VulkanSwapchain::kInvalidSwapchainIndex;
	VulkanSemaphore		m_swapchainImageIsReadySemaphore;

	// Frame level resources
	VulkanFence			m_frameCompletedFence;
	VulkanSemaphore		m_frameCompletedSemaphore;
	VkDescriptorSet		m_frameDescriptorSet;
	VulkanBuffer		m_frameDescriptorBuffer;
	VkCommandBuffer		m_frameCommandBuffer;

	// Main draw resources
	VulkanFramebuffer	m_mainDrawFramebuffer;
	VulkanTexture		m_mainDrawColorMultisampleTexture;
	VulkanTexture		m_mainDrawDepthMultisampleTexture;
	VulkanTexture		m_mainDrawColorResolveTexture;

	// Mesh instance descriptors
	VulkanDescriptorPool m_meshInstanceDescriptorPool;

	// ImGui
	VulkanFramebuffer m_imguiFramebuffer;
};

class VulkanRenderer : public Renderer
{
public:
	VulkanRenderer();

	virtual void Init(Window* pWindow) final;
	virtual void BeginFrame() final;
	virtual void Update(F32 ds) final;
	virtual void Render() final;
	virtual void Shutdown() final;
	virtual void SetCamera(const Camera* pCamera) final;
	virtual F32 GetAspectRatio() const final;

	void InitSwapchain();
	void RefreshSwapchain();

	void InitRenderFrames();
	void DestroyRenderFrames();

	void InitImgui();

	void SetupNewFrame();
	void PresentFinalImage();

	const Camera* m_pMainCamera;
	F32 m_elapsedTimeSeconds;
	F32 m_deltaTimeSeconds;

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

	// ImGui
	VulkanDescriptorPool m_imguiDescriptorPool;
	VulkanRenderPass m_imguiRenderPass;

	// Materials
	VulkanDescriptorSetLayout m_materialDescriptorSetLayout;
	VulkanDescriptorPool m_materialDescriptorPool;

	// Viking Room
	Mesh* m_pVikingRoomMesh;
	VulkanBuffer m_vikingRoomVertexBuffer;
	VulkanBuffer m_vikingRoomIndexBuffer;

	VulkanTexture m_vikingRoomDiffuseTexture;
	VkDescriptorSet m_vikingRoomMaterialDS;

	VkDescriptorSet m_vikingRoomMeshInstanceDS;
	VulkanPipelineLayout m_vikingRoomMainDrawPipelineLayout;
	VulkanPipeline m_vikingRoomMainDrawPipeline;

	VulkanBuffer m_vikingRoomMeshInstanceBuffer;

	VulkanDescriptorSetLayout m_meshInstanceDescriptorSetLayout;
};
