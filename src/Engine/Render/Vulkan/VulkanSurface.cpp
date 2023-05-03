#include "Engine/Render/Vulkan/VulkanSurface.h"
#include "Engine/Render/Vulkan/VulkanInstance.h"

#include "Engine/Core/Assert.h"

VulkanSurface::VulkanSurface(const Window* pWindow)
	:m_pWindow(pWindow)
	,m_vkSurface(VK_NULL_HANDLE)
{
}

VulkanSurface::~VulkanSurface()
{
}

bool VulkanSurface::Setup()
{
	VkWin32SurfaceCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createInfo.pNext = nullptr;
	createInfo.hwnd = m_pWindow->GetHandle();
	createInfo.hinstance = GetModuleHandle(nullptr);

	VULKAN_ASSERT(vkCreateWin32SurfaceKHR(VulkanInstance::GetHandle(), &createInfo, nullptr, &m_vkSurface));
	return true;
}

void VulkanSurface::Teardown()
{
	vkDestroySurfaceKHR(VulkanInstance::GetHandle(), m_vkSurface, nullptr);
}

U32 VulkanSurface::GetWindowWidth() const
{
	return m_pWindow->GetWidth();
}

U32 VulkanSurface::GetWindowHeight() const
{
	return m_pWindow->GetHeight();
}

VkSurfaceKHR VulkanSurface::GetHandle()
{
	return m_vkSurface;
}