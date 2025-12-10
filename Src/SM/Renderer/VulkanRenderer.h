#pragma once

#include "SM/Math.h"
#include "SM/Renderer/Shader.h"
#include "SM/Renderer/Color.h"
#include "SM/StandardTypes.h"

#define VK_NO_PROTOTYPES
#include "ThirdParty/vulkan/vulkan_core.h"

#define VK_EXPORTED_FUNCTION(func)	extern PFN_##func func;
#define VK_GLOBAL_FUNCTION(func)	extern PFN_##func func;
#define VK_INSTANCE_FUNCTION(func)	extern PFN_##func func;
#define VK_DEVICE_FUNCTION(func)	extern PFN_##func func;
#include "SM/Renderer/VulkanFunctionsManifest.inl"

namespace SM
{
    namespace Platform
    {
        struct Window;
    }

    class VulkanRenderer
    {
        public:
        bool Init(Platform::Window* pWindow);
        void RenderFrame();
        void SetClearColor(const ColorF32& color);
        VkFormat FindSupportedFormat(VkFormat* candidates, U32 numCandidates, VkImageTiling tiling, VkFormatFeatureFlags features);

        Platform::Window* m_pWindow = nullptr;
        ColorF32 m_clearColor;

        VkInstance m_instance = VK_NULL_HANDLE;
        VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
        VkPhysicalDeviceProperties m_physicalDeviceProperties;
        VkPhysicalDeviceMemoryProperties m_physicalDeviceMemoryProperties;
        VkDevice m_device = VK_NULL_HANDLE;
        VkSurfaceKHR m_surface = VK_NULL_HANDLE;

        static const I32 kInvalidQueueIndex = -1;
        I32 m_graphicsQueueIndex = kInvalidQueueIndex;
        I32 m_computeQueueIndex = kInvalidQueueIndex;
        I32 m_transferQueueIndex = kInvalidQueueIndex;
        I32 m_presentationQueueIndex = kInvalidQueueIndex;
        VkQueue m_graphicsQueue = VK_NULL_HANDLE;
        VkQueue m_computeQueue = VK_NULL_HANDLE;
        VkQueue m_presentationQueue = VK_NULL_HANDLE;
        VkQueue m_transferQueue = VK_NULL_HANDLE;

        VkCommandPool m_graphicsCommandPool = VK_NULL_HANDLE;

        VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
        VkSurfaceFormatKHR m_swapchainFormat;
        VkExtent2D m_swapchainExtent;
        U32 m_numFramesInFlight = 0;
        VkImage* m_pSwapchainImages;

        VkSampleCountFlagBits m_maxMsaaSamples = VK_SAMPLE_COUNT_1_BIT;
        VkFormat m_defaultDepthFormat = VK_FORMAT_UNDEFINED;
    };
}
