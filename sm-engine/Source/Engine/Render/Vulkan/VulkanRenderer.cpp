#include "Engine/Render/Vulkan/VulkanRenderer.h"
#include "Engine/Config.h"
#include "Engine/Core/Assert.h"
#include "Engine/Core/Debug.h"
#include "Engine/Core/Macros.h"
#include "Engine/Render/Window.h"
#include <set>

VulkanRenderer::VulkanRenderer()
	:m_pWindow(nullptr)
{
}

void VulkanRenderer::Init(Window* pWindow)
{
	SM_ASSERT(pWindow != nullptr);
	m_pWindow = pWindow;

	m_instance.Init();
	m_surface.Init(m_pWindow, m_instance.m_instance);
	m_device.Init(m_instance.m_instance, m_surface.m_surface);
}

void VulkanRenderer::Shutdown()
{
	m_device.Destroy();
	m_surface.Destroy(m_instance.m_instance);
	m_instance.Destroy();
}