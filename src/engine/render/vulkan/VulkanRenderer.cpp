#include "engine/render/vulkan/VulkanRenderer.h"
#include "engine/core/Config.h"
#include "engine/render/Camera.h"
#include "engine/render/Mesh.h"
#include "engine/render/RenderableMesh.h"
#include "engine/render/vulkan/VulkanBuffer.h"
#include "engine/render/vulkan/VulkanCommandPool.h"
#include "engine/render/vulkan/VulkanCommands.h"
#include "engine/render/vulkan/VulkanDescriptorSet.h"
#include "engine/render/vulkan/VulkanDevice.h"
#include "engine/render/vulkan/VulkanInclude.h"
#include "engine/render/vulkan/VulkanInstance.h"
#include "engine/render/vulkan/VulkanPipeline.h"
#include "engine/render/vulkan/VulkanRenderPass.h"
#include "engine/render/vulkan/VulkanSampler.h"
#include "engine/render/vulkan/VulkanSurface.h"
#include "engine/render/vulkan/VulkanSwapchain.h"
#include "engine/render/vulkan/VulkanTexture.h"
#include "engine/math/Mat44.h"

struct MVPUniformBuffer
{
	Mat44 m_model;
	Mat44 m_view;
	Mat44 m_projection;
};

VulkanRenderer::VulkanRenderer(Window* pWindow)
	:m_pWindow(pWindow)
	,m_pSurface(nullptr)
	,m_pDevice(nullptr)
	,m_pSwapchain(nullptr)
	,m_pGraphicsCommandPool(nullptr)
	,m_pRenderPass(nullptr)
	,m_pColorTarget(nullptr)
	,m_pDepthTarget(nullptr)
	,m_pLinearSampler(nullptr)
	,m_pVikingRoomTexture(nullptr)
	,m_pVikingRoomMesh(nullptr)
	,m_pVikingRoomPipeline(nullptr)
	,m_pWorldAxesMesh(nullptr)
	,m_pWorldAxesPipeline(nullptr)
	,m_pDescriptorSetLayout(nullptr)
	,m_pDescriptorPool(nullptr)
	,m_currentFrame(0)
	,m_pCurrentCamera(nullptr)
{
}

VulkanRenderer::~VulkanRenderer()
{
}

bool VulkanRenderer::Setup()
{
	// instance
	VulkanInstance::Setup();

	// surface
	m_pSurface = new VulkanSurface(m_pWindow);
	m_pSurface->Setup();

	// device (physical & logical)
	m_pDevice = new VulkanDevice(m_pSurface);
	m_pDevice->Setup();

	// swapchain
	m_pSwapchain = new VulkanSwapchain(m_pDevice);
	m_pSwapchain->Setup();

	// command pool
	m_pGraphicsCommandPool = new VulkanCommandPool(m_pDevice);
	m_pGraphicsCommandPool->Setup(VK_QUEUE_GRAPHICS_BIT, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	SetupMeshes();
	SetupTexturesAndSamplers();
	SetupUniformBuffers();
	SetupRenderPass();
	SetupFrameBuffers();
	SetupDescriptorSets();
	SetupPipelines();
	SetupCommandBuffers();
	SetupSyncObjects();

	return true;
}

void VulkanRenderer::Teardown()
{
    // don't want to destroy things if we are still in the middle of using them for rendering
	m_pDevice->Flush();

	TeardownSwapchain();

	for (size_t iSyncObj = 0; iSyncObj < MAX_NUM_FRAMES_IN_FLIGHT; iSyncObj++)
	{
		vkDestroySemaphore(m_pDevice->GetHandle(), m_vkImageAvailableSemaphores[iSyncObj], nullptr);
		vkDestroySemaphore(m_pDevice->GetHandle(), m_vkRenderFinishedSemaphores[iSyncObj], nullptr);
		vkDestroyFence(m_pDevice->GetHandle(), m_vkFrameFinishedFences[iSyncObj], nullptr);
	}

	m_pLinearSampler->Teardown();
	delete m_pLinearSampler;

	m_pVikingRoomTexture->Teardown();
	delete m_pVikingRoomTexture;

	m_pVikingRoomMesh->Teardown();
	delete m_pVikingRoomMesh;

	m_pWorldAxesMesh->Teardown();
	delete m_pWorldAxesMesh;

	m_pGraphicsCommandPool->Teardown();
	delete m_pGraphicsCommandPool;

	m_pDevice->Teardown();
	delete m_pDevice;

	m_pSurface->Teardown();
	delete m_pSurface;

	VulkanInstance::Teardown();
}

void VulkanRenderer::TeardownSwapchain()
{
	for (VkFramebuffer& fb : m_vkSwapChainFrameBuffers)
	{
		vkDestroyFramebuffer(m_pDevice->GetHandle(), fb, nullptr);
	}

	vkFreeCommandBuffers(m_pDevice->GetHandle(), m_pGraphicsCommandPool->GetHandle(), (U32)m_vkCommandBuffers.size(), m_vkCommandBuffers.data());

	m_pVikingRoomPipeline->Teardown();
	delete m_pVikingRoomPipeline;

	m_pWorldAxesPipeline->Teardown();
	delete m_pWorldAxesPipeline;

	m_pRenderPass->Teardown();
	delete m_pRenderPass;

	m_pColorTarget->Teardown();
	delete m_pColorTarget;

	m_pDepthTarget->Teardown();
	delete m_pDepthTarget;

	for (U32 i = 0; i < m_pSwapchain->GetNumImages(); i++)
	{
		m_pUniformBuffers[i]->Teardown();
		delete m_pUniformBuffers[i];
	}

	m_pDescriptorPool->Teardown();
	delete m_pDescriptorPool;

	m_pDescriptorSetLayout->Teardown();
	delete m_pDescriptorSetLayout;

	m_pSwapchain->Teardown();
	delete m_pSwapchain;
}

void VulkanRenderer::ResetupSwapChain()
{
	m_pDevice->Flush();

	if (m_pWindow->IsMinimized())
	{
		return;
	}

	TeardownSwapchain();

	m_pSwapchain = new VulkanSwapchain(m_pDevice);
	m_pSwapchain->Setup();

	m_pColorTarget = new VulkanTexture(m_pDevice);
	m_pColorTarget->SetupColorTarget(m_pSwapchain->GetFormat(), m_pSwapchain->GetWidth(), m_pSwapchain->GetHeight());

	m_pDepthTarget = new VulkanTexture(m_pDevice);
	m_pDepthTarget->SetupDepthTarget(m_pDevice->FindDepthFormat(), m_pSwapchain->GetWidth(), m_pSwapchain->GetHeight());
	
	SetupRenderPass();
	SetupFrameBuffers();
	SetupUniformBuffers();
	SetupDescriptorSets();
	SetupPipelines();
	SetupCommandBuffers();
}

void VulkanRenderer::SetupMeshes()
{
	Mesh* pMesh = MeshLoaders::LoadMeshFromObj("Models/viking_room.obj");
	m_pVikingRoomMesh = new RenderableMesh(m_pDevice, pMesh);
	m_pVikingRoomMesh->Setup(m_pGraphicsCommandPool);

	m_pWorldAxesMesh = new RenderableMesh(m_pDevice, MeshLoaders::LoadWorldAxesMesh());
	m_pWorldAxesMesh->Setup(m_pGraphicsCommandPool);
}

void VulkanRenderer::SetupTexturesAndSamplers()
{
	m_pVikingRoomTexture = new VulkanTexture(m_pDevice);
	m_pVikingRoomTexture->SetupFromTextureFile(m_pGraphicsCommandPool, "Textures/viking_room.png");

	m_pColorTarget = new VulkanTexture(m_pDevice);
	m_pColorTarget->SetupColorTarget(m_pSwapchain->GetFormat(), m_pSwapchain->GetWidth(), m_pSwapchain->GetHeight());

	m_pDepthTarget = new VulkanTexture(m_pDevice);
	m_pDepthTarget->SetupDepthTarget(m_pDevice->FindDepthFormat(), m_pSwapchain->GetWidth(), m_pSwapchain->GetHeight());

	m_pLinearSampler = new VulkanSampler(m_pDevice);
	m_pLinearSampler->Setup(m_pVikingRoomTexture->GetNumMipLevels());
}

void VulkanRenderer::SetupUniformBuffers()
{
	m_pUniformBuffers.resize(m_pSwapchain->GetNumImages());
	for (size_t i = 0; i < static_cast<U32>(m_pSwapchain->GetNumImages()); i++)
	{
		m_pUniformBuffers[i] = new VulkanBuffer(m_pDevice);
		m_pUniformBuffers[i]->SetupBuffer(VulkanBufferType::kUniformBuffer, sizeof(MVPUniformBuffer));
	}
}

void VulkanRenderer::SetupRenderPass()
{	
	VulkanRenderPassAttachments passAttachments;
	passAttachments.Add(m_pSwapchain->GetFormat(), m_pDevice->GetMaxMsaaSampleCount(),
						VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
						VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);
	passAttachments.Add(m_pDevice->FindDepthFormat(), m_pDevice->GetMaxMsaaSampleCount(),
						VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
						VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE);
	passAttachments.Add(m_pSwapchain->GetFormat(), VK_SAMPLE_COUNT_1_BIT,
						VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
						VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE);

	VulkanSubpass subpass;
	subpass.AddColorAttachmentRef(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	subpass.AddDepthStencilAttachmentRef(1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	subpass.AddResolveAttachmentRef(2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	m_pRenderPass = new VulkanRenderPass(m_pDevice);
	m_pRenderPass->SetAttachments(passAttachments);
	m_pRenderPass->AddSubpass(subpass);
	m_pRenderPass->AddSubpassDependency(VK_SUBPASS_EXTERNAL, 0, 
										VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, 0, 
										VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);
	m_pRenderPass->Setup();
}

void VulkanRenderer::SetupFrameBuffers()
{
	m_vkSwapChainFrameBuffers.resize(m_pSwapchain->GetNumImages());

	for (U32 i = 0; i < m_pSwapchain->GetNumImages(); i++)
	{
		std::vector<VkImageView> attachments = {
			m_pColorTarget->GetImageView(),			
			m_pDepthTarget->GetImageView(),
			m_pSwapchain->GetImageView(i)
		};

		m_vkSwapChainFrameBuffers[i] = m_pRenderPass->CreateFrameBuffer(attachments, m_pSwapchain->GetWidth(), m_pSwapchain->GetHeight(), 1);
	}
}

void VulkanRenderer::SetupDescriptorSets()
{
	m_pDescriptorSetLayout = new VulkanDescriptorSetLayout(m_pDevice);
	m_pDescriptorSetLayout->AddBinding(0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
	m_pDescriptorSetLayout->AddBinding(1, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
	m_pDescriptorSetLayout->Setup();

	m_pDescriptorPool = new VulkanDescriptorPool(m_pDevice);
	m_pDescriptorPool->AddPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, m_pSwapchain->GetNumImages());
	m_pDescriptorPool->AddPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_pSwapchain->GetNumImages());
	m_pDescriptorPool->Setup(m_pSwapchain->GetNumImages());

	m_vkDescriptorSets = m_pDescriptorPool->AllocateDescriptorSets(m_pDescriptorSetLayout, m_pSwapchain->GetNumImages());

	VulkanDescriptorSetWriter writer(m_pDevice);
	for (size_t i = 0; i < m_pSwapchain->GetNumImages(); ++i)
	{
		writer.Reset();
		writer.AddUniformBufferWrite(m_pUniformBuffers[i], 0, 0);
		writer.AddImageSamplerWrite(m_pVikingRoomTexture, m_pLinearSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1, 0);
		writer.WriteDescriptorSet(m_vkDescriptorSets[i]);
	}
}

void VulkanRenderer::SetupPipelines()
{
	// Viking Room
	VulkanPipelineFactory pipelineMaker(m_pDevice);
	pipelineMaker.SetVsPs("Shaders/tri-vert.spv", "main", "Shaders/tri-frag.spv", "main");
	pipelineMaker.SetVertexInput(m_pVikingRoomMesh, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	pipelineMaker.SetViewportAndScissor(m_pSwapchain->GetWidth(), m_pSwapchain->GetHeight());
	pipelineMaker.SetRasterizer(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
	pipelineMaker.SetMultisampling(m_pDevice->GetMaxMsaaSampleCount());
	pipelineMaker.SetDepthStencil(true, true, VK_COMPARE_OP_LESS, false, 0.0f, 1.0f);
	
	VulkanPipelineColorBlendAttachments colorBlendAttachments;
	colorBlendAttachments.Add(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, false,
							  VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD,
							  VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD);
	pipelineMaker.SetBlend(colorBlendAttachments, false, VK_LOGIC_OP_COPY, 0.0f, 0.0f, 0.0f, 0.0f);

	std::vector<VkDescriptorSetLayout> descriptorSetLayouts = { m_pDescriptorSetLayout->GetHandle() };
	pipelineMaker.SetPipelineLayout(descriptorSetLayouts);

	m_pVikingRoomPipeline = pipelineMaker.CreatePipeline(m_pRenderPass, 0);

	// World axes
	pipelineMaker.Reset();
	pipelineMaker.SetVsPs("Shaders/simple-color-vert.spv", "main", "Shaders/simple-color-frag.spv", "main");
	pipelineMaker.SetVertexInput(m_pWorldAxesMesh, VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
	pipelineMaker.SetViewportAndScissor(m_pSwapchain->GetWidth(), m_pSwapchain->GetHeight());
	pipelineMaker.SetRasterizer(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
	pipelineMaker.SetMultisampling(m_pDevice->GetMaxMsaaSampleCount());
	pipelineMaker.SetDepthStencil(true, true, VK_COMPARE_OP_LESS, false, 0.0f, 1.0f);
	
	colorBlendAttachments.Reset();
	colorBlendAttachments.Add(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, false,
							  VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD,
							  VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD);
	pipelineMaker.SetBlend(colorBlendAttachments, false, VK_LOGIC_OP_COPY, 0.0f, 0.0f, 0.0f, 0.0f);

	descriptorSetLayouts = { m_pDescriptorSetLayout->GetHandle() };
	pipelineMaker.SetPipelineLayout(descriptorSetLayouts);

	m_pWorldAxesPipeline = pipelineMaker.CreatePipeline(m_pRenderPass, 0);
}

void VulkanRenderer::SetupCommandBuffers()
{
	m_vkCommandBuffers = m_pGraphicsCommandPool->AllocateCommandBuffers(VK_COMMAND_BUFFER_LEVEL_PRIMARY, m_pSwapchain->GetNumImages());

	for (size_t i = 0; i < m_vkCommandBuffers.size(); i++)
	{
		VulkanCommands::BeginCommandBuffer(m_vkCommandBuffers[i]);
		
		VkOffset2D offset{ 0, 0 };
		std::vector<VkClearValue> clearColors = { {0.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 0} };
		VulkanCommands::BeginRenderPass(m_vkCommandBuffers[i], m_pRenderPass, m_vkSwapChainFrameBuffers[i], offset, m_pSwapchain->GetExtent(), clearColors);
		{
			// Normal draw
			{
				std::vector<VkDescriptorSet> descriptorSets = { m_vkDescriptorSets[i] };
				VulkanCommands::DrawRenderableMesh(m_vkCommandBuffers[i], m_pVikingRoomMesh, m_pVikingRoomPipeline, descriptorSets);
			}
			 
			// World Axes
			{
				std::vector<VkDescriptorSet> descriptorSets = { m_vkDescriptorSets[i] };
				VulkanCommands::DrawRenderableMesh(m_vkCommandBuffers[i], m_pWorldAxesMesh, m_pWorldAxesPipeline, descriptorSets);
			}
		}
		VulkanCommands::EndRenderPass(m_vkCommandBuffers[i]);

		VulkanCommands::EndCommandBuffer(m_vkCommandBuffers[i]);
	}
}

void VulkanRenderer::SetupSyncObjects()
{
	m_vkImageAvailableSemaphores.resize(MAX_NUM_FRAMES_IN_FLIGHT);
	m_vkRenderFinishedSemaphores.resize(MAX_NUM_FRAMES_IN_FLIGHT);
	m_vkFrameFinishedFences.resize(MAX_NUM_FRAMES_IN_FLIGHT);
	m_vkSwapChainImagesInFlight.resize(static_cast<U32>(m_pSwapchain->GetNumImages()), VK_NULL_HANDLE);

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t iSyncObj = 0; iSyncObj < MAX_NUM_FRAMES_IN_FLIGHT; iSyncObj++)
	{
		VULKAN_ASSERT(vkCreateSemaphore(m_pDevice->GetHandle(), &semaphoreInfo, nullptr, &m_vkImageAvailableSemaphores[iSyncObj]));
		VULKAN_ASSERT(vkCreateSemaphore(m_pDevice->GetHandle(), &semaphoreInfo, nullptr, &m_vkRenderFinishedSemaphores[iSyncObj]));
		VULKAN_ASSERT(vkCreateFence(m_pDevice->GetHandle(), &fenceInfo, nullptr, &m_vkFrameFinishedFences[iSyncObj]));
	}
}

void VulkanRenderer::SetCamera(Camera* pCamera)
{
	m_pCurrentCamera = pCamera;
}

void VulkanRenderer::RenderFrame()
{
	// Wait for previous frame to finish, this blocks on CPU
	vkWaitForFences(m_pDevice->GetHandle(), 1, &m_vkFrameFinishedFences[m_currentFrame], VK_TRUE, UINT64_MAX);

	// Acquire an image from the swap chain
	U32 imageIndex;
	VkResult res = vkAcquireNextImageKHR(m_pDevice->GetHandle(), m_pSwapchain->GetHandle(), UINT64_MAX, m_vkImageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &imageIndex);

	if (res == VK_ERROR_OUT_OF_DATE_KHR)
	{
		TeardownSwapchain();
		return;
	}

	ASSERT(res == VK_SUCCESS || res == VK_SUBOPTIMAL_KHR);

	UpdateUniformBuffer(imageIndex);

	if (m_vkSwapChainImagesInFlight[imageIndex] != VK_NULL_HANDLE)
	{
		vkWaitForFences(m_pDevice->GetHandle(), 1, &m_vkSwapChainImagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
	}

	m_vkSwapChainImagesInFlight[imageIndex] = m_vkFrameFinishedFences[m_currentFrame];

	// Submit command buffer
	VkSubmitInfo submitInfo;
	MemZero(submitInfo);
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { m_vkImageAvailableSemaphores[m_currentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_vkCommandBuffers[imageIndex];

	VkSemaphore signalSemaphores[] = { m_vkRenderFinishedSemaphores[m_currentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	vkResetFences(m_pDevice->GetHandle(), 1, &m_vkFrameFinishedFences[m_currentFrame]);
	VULKAN_ASSERT(vkQueueSubmit(m_pDevice->GetGraphicsQueue(), 1, &submitInfo, m_vkFrameFinishedFences[m_currentFrame]));

	// Present to screen
	VkPresentInfoKHR presentInfo;
	MemZero(presentInfo);
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;
	VkSwapchainKHR swapChains[] = { m_pSwapchain->GetHandle() };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr;

	res = vkQueuePresentKHR(m_pDevice->GetGraphicsQueue(), &presentInfo);

	if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR || m_pWindow->WasResized())
	{
		ResetupSwapChain();
	}
	else
	{
		VULKAN_ASSERT(res);
	}

	m_currentFrame = (m_currentFrame + 1) % MAX_NUM_FRAMES_IN_FLIGHT;
}

void VulkanRenderer::UpdateUniformBuffer(U32 currentImage)
{
	MVPUniformBuffer ubo;

	static F32 dt = 0.0f;
	dt += 0.016f;

	const F32 speed = 0.0f;

	Mat44 rot = Mat44::IDENTITY;
	rot.RotateZDeg(dt * speed);
	ubo.m_model = rot;

	ubo.m_view = m_pCurrentCamera->GetViewTransform();

	ubo.m_projection = CreatePerspectiveProjection(45.0f, 0.01f, 100.0f, m_pSwapchain->GetAspectRatio());

	m_pUniformBuffers[currentImage]->Update(m_pGraphicsCommandPool, &ubo);
}
