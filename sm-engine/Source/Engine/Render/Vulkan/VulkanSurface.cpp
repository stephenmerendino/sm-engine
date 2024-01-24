#include "Engine/Render/Vulkan/VulkanSurface.h"
#include "Engine/Render/Window.h"
#include "Engine/Core/Assert.h"

VulkanSurface::VulkanSurface()
	:m_surface(VK_NULL_HANDLE)
{
}

void VulkanSurface::Init(Window* pWindow, VkInstance instance)
{
	SM_ASSERT(pWindow != nullptr);
	VkWin32SurfaceCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createInfo.pNext = nullptr;
	createInfo.hwnd = pWindow->m_hwnd;
	createInfo.hinstance = GetModuleHandle(nullptr);
	SM_VULKAN_ASSERT(vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &m_surface));
}

void VulkanSurface::Destroy(VkInstance instance)
{
	vkDestroySurfaceKHR(instance, m_surface, nullptr);
}
