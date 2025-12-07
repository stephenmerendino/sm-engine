#pragma once

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
    enum ShaderType
    {
        kVertex,
        kPixel,
        kCompute,
        kNumShaderTypes
    };

    struct Shader
    {
        const char* m_fileName = nullptr; 
        const char* m_entryFunctionName = nullptr;
        Byte* m_byteCode = nullptr;
        ShaderType m_type;
    };

    namespace Platform
    {
        struct Window;
    }

    class VulkanRenderer
    {
        public:
        bool Init(Platform::Window* pWindow);
        VkFormat FindSupportedFormat(VkFormat* candidates, U32 numCandidates, VkImageTiling tiling, VkFormatFeatureFlags features);

        void Test();

        Platform::Window* m_pWindow = nullptr;

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
        static const U32 kMaxNumSwapchainImages = 3;
        VkImage m_swapchainImages[kMaxNumSwapchainImages];
        U32 m_numSwapchainImages = 0;

        VkSampleCountFlagBits m_maxMsaaSamples = VK_SAMPLE_COUNT_1_BIT;
        VkFormat m_defaultDepthFormat = VK_FORMAT_UNDEFINED;
    };
}
