#include "Engine/Render/Vulkan/VulkanRenderer.h"
#include "Engine/Render/Vulkan/VulkanCommands.h"
#include "Engine/Render/Vulkan/VulkanFormats.h"
#include "Engine/Core/Assert.h"
#include "Engine/Core/Debug.h"
#include "Engine/Core/Macros.h"
#include "Engine/Render/Mesh.h"
#include "Engine/Render/Window.h"
#include <set>

struct FrameRenderData
{
	F32 m_elapsedTimeSeconds;
	F32 m_deltaTimeSeconds;
};

VulkanRenderer::VulkanRenderer()
	:m_pWindow(nullptr)
	,m_pMainCamera(nullptr)
	,m_surface(VK_NULL_HANDLE)
	,m_currentFrame(0)
	,m_globalDescriptorSet(VK_NULL_HANDLE)
	,m_elapsedTimeSeconds(0.0f)
	,m_deltaTimeSeconds(0.0f)
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
	VulkanFormats::Init();

	m_graphicsCommandPool.Init(VK_QUEUE_GRAPHICS_BIT, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	InitSwapchain();

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

		m_mainDrawRenderPass.PreInitAddAttachmentDesc(VulkanFormats::GetMainColorFormat(), VK_SAMPLE_COUNT_1_BIT,
													  VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
													  VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, 
													  VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, 0);

		m_mainDrawRenderPass.PreInitAddSubpassAttachmentReference(0, VulkanSubpass::COLOR, 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		m_mainDrawRenderPass.PreInitAddSubpassAttachmentReference(0, VulkanSubpass::DEPTH, 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
		m_mainDrawRenderPass.PreInitAddSubpassAttachmentReference(0, VulkanSubpass::COLOR_RESOLVE, 2, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

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

	// Frame data descriptors
	{
		m_frameDescriptorSetLayout.PreInitAddLayoutBinding(0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL);
		m_frameDescriptorSetLayout.Init();

		m_frameDescriptorPool.PreInitAddPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_NUM_FRAMES_IN_FLIGHT);
		m_frameDescriptorPool.Init(MAX_NUM_FRAMES_IN_FLIGHT);
	}

	InitRenderFrames();
}

void VulkanRenderer::Update(F32 ds)
{
	m_elapsedTimeSeconds += ds;
	m_deltaTimeSeconds = ds;
}

void VulkanRenderer::Render()
{
	/*
	Basic High Level Frame Flow

	DONE 1: Acquire a swapchain image to present at end of frame
	DONE 2: Transition resolve buffers from transfer src to color/depth output
	DONE 3: Update Frame Descriptor Set
	DONE 4: Begin Main Draw Render Pass
		4a: For each mesh
			Update Mesh Descriptor Set with mvp
			Bind Pipeline
			Bind Descriptor Sets
				Global
				Frame
				Material
				Mesh
			Bind Vertex Buffer
			Bind Index Buffer
			Draw Command
	DONE 5: Transition Color Resolve to Transfer Src
	DONE 6: Transition Swapchain Image to Transfer Dst
	DONE 7: Copy Color Resolve to Swapchain Image
	DONE 8: Transition Swapchain Image to Present
	DONE 9: Present Swapchain Image
	*/

	SetupNewFrame();
	VulkanRenderFrame& curRenderFrame = m_renderFrames[m_currentFrame];

	VulkanCommands::Begin(curRenderFrame.m_frameCommandBuffer, 0);

	// Main Draw
	{
		VkClearValue colorClear = {};
		colorClear.color = { 0.25f, 0.25f, 0.25f, 1.0f };
		VkClearValue depthClear = {};
		depthClear.depthStencil = { 1.0f, 0 };

		std::vector<VkClearValue> mainDrawClearValues = { colorClear, depthClear, colorClear };

		VkOffset2D mainDrawOffset = { 0, 0 };
		VkExtent2D mainDrawExtent = m_swapchain.m_extent;

		VulkanCommands::BeginRenderPass(curRenderFrame.m_frameCommandBuffer, m_mainDrawRenderPass.m_renderPassHandle, curRenderFrame.m_mainDrawFramebuffer.m_framebufferHandle, mainDrawOffset, mainDrawExtent, mainDrawClearValues);
			// Draw meshes here
		VulkanCommands::EndRenderPass(curRenderFrame.m_frameCommandBuffer);

	}

	// Copy from main draw target to swapchain
	{
		VulkanCommands::TransitionImageLayout(curRenderFrame.m_frameCommandBuffer, m_swapchain.m_images[curRenderFrame.m_swapchainImageIndex], 1,
											  VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
											  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
											  VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT);

		VulkanCommands::CopyImage(curRenderFrame.m_frameCommandBuffer, curRenderFrame.m_mainDrawColorResolveTexture.m_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
								  m_swapchain.m_images[curRenderFrame.m_swapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
								  m_swapchain.m_extent.width, m_swapchain.m_extent.height, 1);

		VulkanCommands::TransitionImageLayout(curRenderFrame.m_frameCommandBuffer, m_swapchain.m_images[curRenderFrame.m_swapchainImageIndex], 1,
											  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
											  VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_NONE,
											  VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_NONE);
	}

	VulkanCommands::End(curRenderFrame.m_frameCommandBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = nullptr;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &curRenderFrame.m_swapchainImageIsReadySemaphore.m_semaphoreHandle;
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_TRANSFER_BIT };
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &curRenderFrame.m_frameCommandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &curRenderFrame.m_frameCompletedSemaphore.m_semaphoreHandle;
	vkQueueSubmit(VulkanDevice::Get()->m_graphicsQueue, 1, &submitInfo, curRenderFrame.m_frameCompletedFence.m_fenceHandle);

	PresentFinalImage();
}

void VulkanRenderer::SetupNewFrame()
{
	m_currentFrame = (m_currentFrame + 1) % MAX_NUM_FRAMES_IN_FLIGHT;
	VulkanRenderFrame& curRenderFrame = m_renderFrames[m_currentFrame];

	// Make sure frame has finished from last use before using it for new frame
	curRenderFrame.m_frameCompletedFence.Wait();
	curRenderFrame.m_frameCompletedFence.Reset();

	vkResetCommandBuffer(curRenderFrame.m_frameCommandBuffer, 0);

	// Acquire swapchain image to render to
	U32 swapchainImageIndex = VulkanSwapchain::kInvalidSwapchainIndex;
	VkResult result = m_swapchain.AcquireNextImage(&curRenderFrame.m_swapchainImageIsReadySemaphore, nullptr, &swapchainImageIndex);
	if (m_swapchain.NeedsRefresh(result))
	{
		RefreshSwapchain();
	}
	curRenderFrame.m_swapchainImageIndex = swapchainImageIndex;

	// Update frame descriptor
	FrameRenderData frameRenderData;
	frameRenderData.m_elapsedTimeSeconds = m_elapsedTimeSeconds;
	frameRenderData.m_deltaTimeSeconds = m_deltaTimeSeconds;

	curRenderFrame.m_frameDescriptorBuffer.Update(m_graphicsCommandPool, &frameRenderData, 0);

	VulkanDescriptorSetWriter descriptorWriter;
	descriptorWriter.AddUniformBufferWrite(curRenderFrame.m_frameDescriptorSet, curRenderFrame.m_frameDescriptorBuffer, 0, 0, 0, 1);
	descriptorWriter.PerformWrites();
}

void VulkanRenderer::PresentFinalImage()
{
	if (m_pWindow->m_bWasClosed)
	{
		return;
	}

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = nullptr;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &m_renderFrames[m_currentFrame].m_frameCompletedSemaphore.m_semaphoreHandle;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &m_swapchain.m_swapchainHandle;
	presentInfo.pImageIndices = &m_renderFrames[m_currentFrame].m_swapchainImageIndex;
	presentInfo.pResults = nullptr;
	VkResult presentResult = vkQueuePresentKHR(VulkanDevice::Get()->m_graphicsQueue, &presentInfo);
	if (m_swapchain.NeedsRefresh(presentResult))
	{
		RefreshSwapchain();
	}
	else
	{
		SM_VULKAN_ASSERT(presentResult);
	}
}

void VulkanRenderer::Shutdown()
{
	vkQueueWaitIdle(VulkanDevice::Get()->m_graphicsQueue);

	DestroyRenderFrames();

	m_frameDescriptorPool.Destroy();
	m_frameDescriptorSetLayout.Destroy();

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

void VulkanRenderer::InitSwapchain()
{
	m_swapchain.Init(m_pWindow, m_surface);

	VkCommandBuffer commandBuffer = m_graphicsCommandPool.BeginSingleTime();
	m_swapchain.AddInitialImageLayoutTransitionCommands(commandBuffer);
	m_graphicsCommandPool.EndAndSubmitSingleTime(commandBuffer);
}

void VulkanRenderer::RefreshSwapchain()
{
	m_swapchain.Destroy();
	InitSwapchain();
	DestroyRenderFrames();
	InitRenderFrames();
}

void VulkanRenderer::InitRenderFrames()
{
	for (int i = 0; i < MAX_NUM_FRAMES_IN_FLIGHT; i++)
	{
		VulkanRenderFrame& frame = m_renderFrames[i];
		frame.m_swapchainImageIsReadySemaphore.Init();
		frame.m_frameCompletedSemaphore.Init();
		frame.m_frameCompletedFence.Init();
		frame.m_frameCommandBuffer = m_graphicsCommandPool.AllocateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
		frame.m_frameDescriptorSet = m_frameDescriptorPool.AllocateSet(m_frameDescriptorSetLayout);
		frame.m_frameDescriptorBuffer.Init(VulkanBuffer::Type::kUniformBuffer, sizeof(FrameRenderData));

		frame.m_mainDrawColorMultisampleTexture.InitColorTarget(VulkanFormats::GetMainColorFormat(), m_swapchain.m_extent.width, m_swapchain.m_extent.height, 
										 VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VulkanDevice::Get()->m_maxNumMsaaSamples);

		frame.m_mainDrawDepthMultisampleTexture.InitDepthTarget(VulkanFormats::GetMainDepthFormat(), m_swapchain.m_extent.width, m_swapchain.m_extent.height, 
										 VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VulkanDevice::Get()->m_maxNumMsaaSamples);

		frame.m_mainDrawColorResolveTexture.InitColorTarget(VulkanFormats::GetMainColorFormat(), m_swapchain.m_extent.width, m_swapchain.m_extent.height, 
										 VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_SAMPLE_COUNT_1_BIT);

		std::vector<VkImageView> mainDrawAttachments = {
			frame.m_mainDrawColorMultisampleTexture.m_imageView,
			frame.m_mainDrawDepthMultisampleTexture.m_imageView,
			frame.m_mainDrawColorResolveTexture.m_imageView
		};

		frame.m_mainDrawFramebuffer.Init(m_mainDrawRenderPass, mainDrawAttachments, m_swapchain.m_extent.width, m_swapchain.m_extent.height, 1);
	}
}

void VulkanRenderer::DestroyRenderFrames()
{
	for (int i = 0; i < MAX_NUM_FRAMES_IN_FLIGHT; i++)
	{
		VulkanRenderFrame& frame = m_renderFrames[i];
		frame.m_frameCompletedFence.Destroy();
		frame.m_frameCompletedSemaphore.Destroy();
		frame.m_frameDescriptorBuffer.Destroy();
		frame.m_swapchainImageIsReadySemaphore.Destroy();
		frame.m_mainDrawFramebuffer.Destroy();
		frame.m_mainDrawColorMultisampleTexture.Destroy();
		frame.m_mainDrawDepthMultisampleTexture.Destroy();
		frame.m_mainDrawColorResolveTexture.Destroy();
	}
	m_frameDescriptorPool.Reset();
}
