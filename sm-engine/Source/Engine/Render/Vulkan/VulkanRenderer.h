#pragma once

#include "Engine/Render/Renderer.h"
#include "Engine/Core/Types.h"
#include "Engine/Render/Vulkan/VulkanCommandPool.h"
#include "Engine/Render/Vulkan/VulkanDevice.h"
#include "Engine/Render/Vulkan/VulkanInclude.h"
#include "Engine/Render/Vulkan/VulkanInstance.h"
#include "Engine/Render/Vulkan/VulkanSwapchain.h"
#include <vector>

class Camera;

class VulkanRenderer : public Renderer
{
public:
	VulkanRenderer();

	virtual void Init(Window* pWindow) final;
	virtual void Shutdown() final;
	virtual void SetCamera(const Camera* pCamera) final;

	Window* m_pWindow;
	VkSurfaceKHR m_surface;
	VulkanSwapchain m_swapchain;

	VulkanCommandPool m_graphicsCommandPool;

	const Camera* m_pMainCamera;
};
