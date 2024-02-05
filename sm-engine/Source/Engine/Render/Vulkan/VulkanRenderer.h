#pragma once

#include "Engine/Render/Renderer.h"
#include "Engine/Core/Types.h"
#include "Engine/Render/Vulkan/VulkanCommandPool.h"
#include "Engine/Render/Vulkan/VulkanDevice.h"
#include "Engine/Render/Vulkan/VulkanInclude.h"
#include "Engine/Render/Vulkan/VulkanInstance.h"
#include "Engine/Render/Vulkan/VulkanSurface.h"
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
	VulkanInstance m_instance;
	VulkanSurface m_surface;
	VulkanDevice m_device;
	VulkanCommandPool m_graphicsCommandPool;
	VulkanSwapchain m_swapchain;

	const Camera* m_pMainCamera;
};
