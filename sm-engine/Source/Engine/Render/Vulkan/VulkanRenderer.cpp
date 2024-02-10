#include "Engine/Render/Vulkan/VulkanRenderer.h"
#include "Engine/Render/Vulkan/VulkanCommands.h"
#include "Engine/Render/Vulkan/VulkanFormats.h"
#include "Engine/Core/Assert.h"
#include "Engine/Core/Debug.h"
#include "Engine/Core/Macros.h"
#include "Engine/Render/Camera.h"
#include "Engine/Render/MeshBuilder.h"
#include "Engine/Render/Mesh.h"
#include "Engine/Render/Window.h"
#include <set>

struct FrameRenderData
{
	F32 m_elapsedTimeSeconds;
	F32 m_deltaTimeSeconds;
};

struct MeshInstanceRenderData
{
	Mat44 m_mvp;
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

	// Materials
	m_materialDescriptorSetLayout.PreInitAddLayoutBinding(0, 1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT);
	m_materialDescriptorSetLayout.Init();

	m_materialDescriptorPool.PreInitAddPoolSize(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 20);
	m_materialDescriptorPool.Init(20);

	// Mesh instance
	m_meshInstanceDescriptorSetLayout.PreInitAddLayoutBinding(0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL);
	m_meshInstanceDescriptorSetLayout.Init();

	// Viking Room
	m_pVikingRoomMesh = MeshBuilder::BuildFromObj("viking_room.obj");

	m_vikingRoomVertexBuffer.Init(VulkanBuffer::Type::kVertexBuffer, m_pVikingRoomMesh->CalcVertexBufferSize());
	m_vikingRoomVertexBuffer.Update(m_graphicsCommandPool, m_pVikingRoomMesh->m_vertices.data(), 0);

	m_vikingRoomIndexBuffer.Init(VulkanBuffer::Type::kIndexBuffer, m_pVikingRoomMesh->CalcIndexBufferSize());
	m_vikingRoomIndexBuffer.Update(m_graphicsCommandPool, m_pVikingRoomMesh->m_indices.data(), 0);

	m_vikingRoomDiffuseTexture.InitFromFile(m_graphicsCommandPool, "viking-room.png");

	m_vikingRoomMaterialDS = m_materialDescriptorPool.AllocateSet(m_materialDescriptorSetLayout);

	VulkanDescriptorSetWriter dsWriter;
	dsWriter.AddSampledImageWrite(m_vikingRoomMaterialDS, m_vikingRoomDiffuseTexture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 0, 1);
	dsWriter.PerformWrites();

	m_vikingRoomMeshInstanceBuffer.Init(VulkanBuffer::Type::kUniformBuffer, sizeof(MeshInstanceRenderData));

	// Viking Room Pipeline
	VulkanShaderStages shaderStages;
	shaderStages.Init("simple-diffuse.vert.spv", "main", "simple-diffuse.frag.spv", "main");

	std::vector<VkDescriptorSetLayout> pipelineDescriptorSetLayouts = {
		m_globalDescriptorSetLayout.m_layoutHandle,
		m_frameDescriptorSetLayout.m_layoutHandle,
		m_materialDescriptorSetLayout.m_layoutHandle,
		m_meshInstanceDescriptorSetLayout.m_layoutHandle
	};

	m_vikingRoomMainDrawPipelineLayout.Init(pipelineDescriptorSetLayouts);

	VulkanMeshPipelineInputInfo pipelineMeshInputInfo;
	pipelineMeshInputInfo.Init(m_pVikingRoomMesh, false);

	VulkanPipelineState pipelineState;
	pipelineState.InitRasterState(VK_POLYGON_MODE_FILL, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_CULL_MODE_BACK_BIT);
	pipelineState.InitViewportState(0, 0, (F32)m_swapchain.m_extent.width, (F32)m_swapchain.m_extent.height);
	pipelineState.InitMultisampleState(VulkanDevice::Get()->m_maxNumMsaaSamples);
	pipelineState.InitDepthStencilState(true, true, VK_COMPARE_OP_LESS);
	pipelineState.PreInitAddColorBlendAttachment(false);
	pipelineState.InitColorBlendState(false);

	m_vikingRoomMainDrawPipeline.Init(shaderStages, m_vikingRoomMainDrawPipelineLayout, pipelineMeshInputInfo, pipelineState, m_mainDrawRenderPass);
}

void VulkanRenderer::Update(F32 ds)
{
	m_elapsedTimeSeconds += ds;
	m_deltaTimeSeconds = ds;
}

void VulkanRenderer::Render()
{
	SetupNewFrame();
	VulkanRenderFrame& curRenderFrame = m_renderFrames[m_currentFrame];

	VulkanCommands::Begin(curRenderFrame.m_frameCommandBuffer, 0);

	// Main Draw
	{
		VkClearValue colorClear = {};
		colorClear.color = { CLEAR_COLOR.r, CLEAR_COLOR.g, CLEAR_COLOR.b, CLEAR_COLOR.a };
		VkClearValue depthClear = {};
		depthClear.depthStencil = { 1.0f, 0 };

		std::vector<VkClearValue> mainDrawClearValues = { colorClear, depthClear, colorClear };

		VkOffset2D mainDrawOffset = { 0, 0 };
		VkExtent2D mainDrawExtent = m_swapchain.m_extent;

		VulkanCommands::BeginRenderPass(curRenderFrame.m_frameCommandBuffer, m_mainDrawRenderPass.m_renderPassHandle, curRenderFrame.m_mainDrawFramebuffer.m_framebufferHandle, mainDrawOffset, mainDrawExtent, mainDrawClearValues);

            Mat44 model = Mat44::IDENTITY;
			Mat44 view = m_pMainCamera->GetViewTransform();
			Mat44 projection = Mat44::CreatePerspectiveProjection(45.0f, 0.01f, 100.0f, (F32)m_swapchain.m_extent.width / (F32)m_swapchain.m_extent.height);

            MeshInstanceRenderData meshInstanceRenderData;
			meshInstanceRenderData.m_mvp = model * view * projection;
            m_vikingRoomMeshInstanceBuffer.Update(m_graphicsCommandPool, &meshInstanceRenderData, 0);

			// Mesh instance descriptor set
            VkDescriptorSet meshInstanceDescriptorSet = curRenderFrame.m_meshInstanceDescriptorPool.AllocateSet(m_meshInstanceDescriptorSetLayout);
			VulkanDescriptorSetWriter meshInstanceDescriptorWriter;
			meshInstanceDescriptorWriter.AddUniformBufferWrite(meshInstanceDescriptorSet, m_vikingRoomMeshInstanceBuffer, 0, 0, 0, 1);
			meshInstanceDescriptorWriter.PerformWrites();

			// Bind vertex buffer
			VkBuffer vertexBuffer[] = { m_vikingRoomVertexBuffer.m_bufferHandle };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(curRenderFrame.m_frameCommandBuffer, 0, 1, vertexBuffer, offsets);

			// Bind index buffer
			vkCmdBindIndexBuffer(curRenderFrame.m_frameCommandBuffer, m_vikingRoomIndexBuffer.m_bufferHandle, 0, VK_INDEX_TYPE_UINT32);

			// Bind pipeline
			vkCmdBindPipeline(curRenderFrame.m_frameCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vikingRoomMainDrawPipeline.m_pipelineHandle);

			std::vector<VkDescriptorSet> mainDrawDescriptorSets = {
				m_globalDescriptorSet,
				curRenderFrame.m_frameDescriptorSet,
				m_vikingRoomMaterialDS,
				meshInstanceDescriptorSet	
			};
			vkCmdBindDescriptorSets(curRenderFrame.m_frameCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vikingRoomMainDrawPipelineLayout.m_layoutHandle, 0, (U32)mainDrawDescriptorSets.size(), mainDrawDescriptorSets.data(), 0, nullptr);

			// Draw command
			vkCmdDrawIndexed(curRenderFrame.m_frameCommandBuffer, (U32)m_pVikingRoomMesh->m_indices.size(), 1, 0, 0, 0);

		VulkanCommands::EndRenderPass(curRenderFrame.m_frameCommandBuffer);

	}

	// Copy from main draw target to swapchain
	{
		VulkanCommands::TransitionImageLayout(curRenderFrame.m_frameCommandBuffer, m_swapchain.m_images[curRenderFrame.m_swapchainImageIndex], 1,
											  VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
											  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
											  VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT);

		VulkanCommands::BlitColorImage(curRenderFrame.m_frameCommandBuffer, curRenderFrame.m_mainDrawColorResolveTexture.m_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_swapchain.m_extent.width, m_swapchain.m_extent.height, 1, 0,
                                                                            m_swapchain.m_images[curRenderFrame.m_swapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_swapchain.m_extent.width, m_swapchain.m_extent.height, 1, 0);

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

	// Mesh instance reset
	curRenderFrame.m_meshInstanceDescriptorPool.Reset();
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

F32 VulkanRenderer::GetAspectRatio() const
{
	if (m_swapchain.m_swapchainHandle == VK_NULL_HANDLE)
	{
		return 0.0f;
	}

	return (F32)m_swapchain.m_extent.width / (F32)m_swapchain.m_extent.height;
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
	vkQueueWaitIdle(VulkanDevice::Get()->m_graphicsQueue);
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

		frame.m_meshInstanceDescriptorPool.PreInitAddPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100);
		frame.m_meshInstanceDescriptorPool.Init(100);
	}
}

void VulkanRenderer::InitImgui()
{

}

void VulkanRenderer::DestroyRenderFrames()
{
	for (int i = 0; i < MAX_NUM_FRAMES_IN_FLIGHT; i++)
	{
		VulkanRenderFrame& frame = m_renderFrames[i];
		frame.m_meshInstanceDescriptorPool.Destroy();
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
