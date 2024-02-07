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
	,m_surface(VK_NULL_HANDLE)
	,m_currentFrame(0)
	,m_globalDescriptorSet(VK_NULL_HANDLE)
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

	// Main draw render pass
	{
		m_mainDrawRenderPass.PreInitAddAttachmentDesc(VulkanFormats::GetMainColorFormat(), VulkanDevice::Get()->m_maxNumMsaaSamples,
													  VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
													  VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, 
													  VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, 0);

		m_mainDrawRenderPass.PreInitAddAttachmentDesc(VulkanFormats::GetMainDepthFormat(), VulkanDevice::Get()->m_maxNumMsaaSamples,
													  VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
													  VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, 
													  VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, 0);

		m_mainDrawRenderPass.PreInitAddSubpassAttachmentReference(0, VulkanSubpass::COLOR, 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		m_mainDrawRenderPass.PreInitAddSubpassAttachmentReference(0, VulkanSubpass::DEPTH, 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

		m_mainDrawRenderPass.PreInitAddSubpassDependency(VK_SUBPASS_EXTERNAL, 0,
														 VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
														 VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
														 0, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, 
														 0);

		m_mainDrawRenderPass.Init();
	}

	// Global data descriptors
	{
		m_globalDescriptorSetLayout.PreInitAddLayoutBinding(0, 1, VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_ALL);
		m_globalDescriptorSetLayout.Init();

		m_globalDescriptorPool.PreInitAddPoolSize(VK_DESCRIPTOR_TYPE_SAMPLER, 1);
		m_globalDescriptorPool.Init(1);

		m_globalDescriptorSet = m_globalDescriptorPool.AllocateSet(m_globalDescriptorSetLayout);

		m_globalLinearSampler.Init(10);

		VulkanDescriptorSetWriter globalDescriptorSetWriter;
		globalDescriptorSetWriter.AddSamplerWrite(m_globalDescriptorSet, m_globalLinearSampler, 0, 0, 1);
		globalDescriptorSetWriter.PerformWrites();
	}
}

void VulkanRenderer::RenderFrame()
{
	m_currentFrame = (m_currentFrame + 1) % MAX_NUM_FRAMES_IN_FLIGHT;
}

void VulkanRenderer::Shutdown()
{
	m_globalLinearSampler.Destroy();
	m_globalDescriptorPool.Destroy();
	m_globalDescriptorSetLayout.Destroy();

	m_mainDrawRenderPass.Destroy();
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
