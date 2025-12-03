#pragma once

#include "SM/Renderer/Renderer.h"

#define VK_NO_PROTOTYPES
#include "ThirdParty/vulkan/vulkan_core.h"

#define VK_EXPORTED_FUNCTION(func)	extern PFN_##func func;
#define VK_GLOBAL_FUNCTION(func)	extern PFN_##func func;
#define VK_INSTANCE_FUNCTION(func)	extern PFN_##func func;
#define VK_DEVICE_FUNCTION(func)	extern PFN_##func func;
#include "SM/Renderer/VulkanFunctionsManifest.inl"

namespace SM
{
    class VulkanRenderer : public Renderer
    {
        public:
        // main renderer interface
        virtual bool Init(Platform::Window* pWindow) final;

        // vulkan specific
        VkFormat FindSupportedFormat(VkFormat* candidates, U32 numCandidates, VkImageTiling tiling, VkFormatFeatureFlags features);

        VkInstance m_instance = VK_NULL_HANDLE;
        VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
        VkPhysicalDeviceProperties m_physicalDeviceProperties;
        VkPhysicalDeviceMemoryProperties m_physicalDeviceMemoryProperties;
        VkDevice m_device = VK_NULL_HANDLE;
        VkSurfaceKHR m_surface = VK_NULL_HANDLE;

        VkQueue m_graphicsQueue = VK_NULL_HANDLE;
        VkQueue m_computeQueue = VK_NULL_HANDLE;
        VkQueue m_presentationQueue = VK_NULL_HANDLE;
        VkQueue m_transferQueue = VK_NULL_HANDLE;

        VkSampleCountFlagBits m_maxMsaaSamples = VK_SAMPLE_COUNT_1_BIT;
        VkFormat m_defaultDepthFormat = VK_FORMAT_UNDEFINED;

        VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
        static const int kMaxNumSwapchainImages = 3;
        VkImage m_swapchainImages[kMaxNumSwapchainImages];
        U32 m_numSwapchainImages = 0;
    };
}
