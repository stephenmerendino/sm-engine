#pragma once

#include "SM/Math.h"
#include "SM/Renderer/Shader.h"
#include "SM/Renderer/Color.h"
#include "SM/Renderer/VulkanConfig.h"
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

    class FrameResources;

    class VulkanRenderer
    {
        public:
        bool Init(Platform::Window* pWindow);
        void RenderFrame();
        void SetClearColor(const ColorF32& color);
        VkFormat FindSupportedFormat(VkFormat* candidates, U32 numCandidates, VkImageTiling tiling, VkFormatFeatureFlags features);
        void CreateSwapchain();
        void UpdateSwapchain();

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
        VkImage* m_pSwapchainImages = nullptr;

        U32 m_numFramesInFlight = 0;
        U32 m_curFrameInFlight = 0;
        FrameResources* m_pFrameResources = nullptr;

        VkSampleCountFlagBits m_maxMsaaSamples = VK_SAMPLE_COUNT_1_BIT;
        VkFormat m_defaultDepthFormat = VK_FORMAT_UNDEFINED;

        static const size_t kMaxNumFramesInFlight = VulkanConfig::kOptimalNumFramesInFlight;
    };

    class FrameResources
    {
        public:
        void Init(VulkanRenderer* pRenderer);
        void Update();

        VkExtent2D m_curScreenResolution = { .width = 0, .height = 0 };
        VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
        VkImage m_mainColorRenderTarget = VK_NULL_HANDLE;
        VkDeviceMemory m_mainColorRenderTargetMemory = VK_NULL_HANDLE;

        VulkanRenderer* m_pRenderer = nullptr;
    };

    inline bool operator==(const VkExtent2D& a, const VkExtent2D& b)
    {
        return a.width == b.width && a.height == b.height;
    }

    inline bool operator!=(const VkExtent2D& a, const VkExtent2D& b)
    {
        return !(a == b);
    }
}
