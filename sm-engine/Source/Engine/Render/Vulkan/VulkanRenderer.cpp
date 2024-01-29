#include "Engine/Render/Vulkan/VulkanRenderer.h"
#include "Engine/Render/Vulkan/VulkanCommands.h"
#include "Engine/Config.h"
#include "Engine/Core/Assert.h"
#include "Engine/Core/Debug.h"
#include "Engine/Core/Macros.h"
#include "Engine/Render/Mesh.h"
#include "Engine/Render/Window.h"
#include <set>

VulkanRenderer::VulkanRenderer()
	:m_pWindow(nullptr)
	,m_pMainCamera(nullptr)
{
}

void VulkanRenderer::Init(Window* pWindow)
{
	SM_ASSERT(pWindow != nullptr);
	m_pWindow = pWindow;

	m_instance.Init();
	m_surface.Init(m_pWindow, m_instance.m_instanceHandle);
	m_device.Init(m_instance.m_instanceHandle, m_surface.m_surfaceHandle);
	m_graphicsCommandPool.Init(&m_device, VK_QUEUE_GRAPHICS_BIT, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	m_swapchain.Init(m_pWindow, m_surface.m_surfaceHandle, m_device, m_graphicsCommandPool);

	Mesh::LoadPrimitives();
}

void VulkanRenderer::Shutdown()
{
	m_swapchain.Destroy(m_device.m_deviceHandle);
	m_graphicsCommandPool.Destroy();
	m_device.Destroy();
	m_surface.Destroy(m_instance.m_instanceHandle);
	m_instance.Destroy();
}

void VulkanRenderer::SetCamera(const Camera* pCamera)
{
	m_pMainCamera = pCamera;
}
