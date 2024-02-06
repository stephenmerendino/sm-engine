#include "Engine/Render/Vulkan/VulkanRenderer.h"
#include "Engine/Render/Vulkan/VulkanCommands.h"
#include "Engine/Render/Vulkan/VulkanFormats.h"
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

	Mesh::InitPrimitives();

	VulkanInstance::Init();

	// Create surface to render to in the window
	VkWin32SurfaceCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createInfo.pNext = nullptr;
	createInfo.hwnd = pWindow->m_hwnd;
	createInfo.hinstance = GetModuleHandle(nullptr);
	SM_VULKAN_ASSERT(vkCreateWin32SurfaceKHR(VulkanInstance::GetHandle(), &createInfo, nullptr, &m_surface));

	VulkanDevice::Init(m_surface);
	m_swapchain.Init(m_pWindow, m_surface);

	VulkanFormats::Init();

	m_graphicsCommandPool.Init(VK_QUEUE_GRAPHICS_BIT, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	// Initialize swapchain images to correct layout
	{
        VkCommandBuffer commandBuffer = m_graphicsCommandPool.BeginSingleTime();
        m_swapchain.AddInitialImageLayoutTransitionCommands(commandBuffer);
        m_graphicsCommandPool.EndAndSubmitSingleTime(commandBuffer);
	}

	/*
	VulkanRenderPass renderPass;
	renderPass.PreInitAddAttachment(....);
	renderPass.PreInitAddAttachment(....);
	renderPass.PreInitAddSubpassAttachmentReference(....);
	renderPass.PreInitAddSubpassAttachmentReference(....);
	renderPass.PreInitAddSubpassDependency(....);
	renderPass.Init();
	*/
}

void VulkanRenderer::Shutdown()
{
	m_swapchain.Destroy();
	m_graphicsCommandPool.Destroy();
	VulkanDevice::Destroy();
	vkDestroySurfaceKHR(VulkanInstance::GetHandle(), m_surface, nullptr);
	VulkanInstance::Destroy();
}

void VulkanRenderer::SetCamera(const Camera* pCamera)
{
	m_pMainCamera = pCamera;
}
