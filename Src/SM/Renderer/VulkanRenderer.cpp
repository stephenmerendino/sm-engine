#include "SM/Renderer/VulkanRenderer.h"
#include "SM/Assert.h"
#include "SM/Bits.h"
#include "SM/Math.h"
#include "SM/Memory.h"
#include "SM/Renderer/VulkanConfig.h"
#include "ThirdParty/vulkan/vulkan_core.h"

#include <cstring>

using namespace SM;

Vec3 MakeColorNormalized(U8 r, U8 g, U8 b)
{
    return Vec3((F32)r / 255.f, (F32)g / 255.f, (F32)b / 255.f);
}

static const Vec3 kDefaultClearColor = MakeColorNormalized(0, 100, 100);

struct Color
{
    Color(F32 _r, F32 _g, F32 _b, F32 _a = 1.0f);
    Color(U8 _r, U8 _g, U8 _b, U8 _a = 255);

    F32 r = 0.0f;
    F32 g = 0.0f;
    F32 b = 0.0f;
    F32 a = 0.0f;
};

bool VulkanRenderer::Init(Platform::Window* pWindow)
{
    SM::PushAllocator(kEngineGlobal);

    m_pWindow = pWindow;
    m_clearColor = ColorF32(0, 100, 100);

    Platform::LoadVulkanGlobalFuncs();

    //------------------------------------------------------------------------------------------------------------------------
    // Instance
    //------------------------------------------------------------------------------------------------------------------------
    VkApplicationInfo appInfo {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "SM Workbench",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "SM Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_3
    };

    VkInstanceCreateInfo instanceCreateInfo {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
        .enabledExtensionCount = ARRAY_LEN(INSTANCE_EXTENSIONS),
        .ppEnabledExtensionNames = INSTANCE_EXTENSIONS
    };

    if (ENABLE_VALIDATION_LAYERS)
    {
        // check validation layers are supported
        {
            U32 numLayers;
            vkEnumerateInstanceLayerProperties(&numLayers, nullptr);

            VkLayerProperties* instanceLayers = SM::Alloc<VkLayerProperties>(numLayers);
            vkEnumerateInstanceLayerProperties(&numLayers, instanceLayers);

            for (const char* layerName : VALIDATION_LAYERS)
            {
                bool layerFound = false;

                for (U32 i = 0; i < numLayers; i++)
                {
                    const VkLayerProperties& l = instanceLayers[i];
                    if (strcmp(layerName, l.layerName) == 0)
                    {
                        layerFound = true;
                        break;
                    }
                }

                SM_ASSERT(layerFound);
            }
        }

        instanceCreateInfo.ppEnabledLayerNames = VALIDATION_LAYERS;
        instanceCreateInfo.enabledLayerCount = ARRAY_LEN(VALIDATION_LAYERS);

        // this debug messenger debugs the actual instance creation
        VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,

            .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,

            .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,

            .pfnUserCallback = Platform::GetVulkanDebugCallback(),
            .pUserData = nullptr
        };
        instanceCreateInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugMessengerCreateInfo;

        if (ENABLE_VALIDATION_BEST_PRACTICES)
        {
            VkValidationFeatureEnableEXT enables[] = { VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT };
            VkValidationFeaturesEXT features {};
            features.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
            features.enabledValidationFeatureCount = 1;
            features.pEnabledValidationFeatures = enables;
            debugMessengerCreateInfo.pNext = &features;
        }
    }

    SM_ASSERT(vkCreateInstance(&instanceCreateInfo, nullptr, &m_instance) == VK_SUCCESS);
    Platform::LoadVulkanInstanceFuncs(m_instance);

    //------------------------------------------------------------------------------------------------------------------------
    // Debug Messenger
    //------------------------------------------------------------------------------------------------------------------------
    if (ENABLE_VALIDATION_LAYERS)
    {
        VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,

            .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,

            .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,

            .pfnUserCallback = Platform::GetVulkanDebugCallback(),
            .pUserData = nullptr
        };
        VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
        SM_ASSERT(vkCreateDebugUtilsMessengerEXT(m_instance, &debugMessengerCreateInfo, nullptr, &debugMessenger) == VK_SUCCESS);
    }

    m_surface = Platform::CreateVulkanSurface(m_instance, pWindow);

    //------------------------------------------------------------------------------------------------------------------------
    // Physical Device
    //------------------------------------------------------------------------------------------------------------------------
    U32 numFoundGPUs = 0;
    vkEnumeratePhysicalDevices(m_instance, &numFoundGPUs, nullptr);
    SM_ASSERT(numFoundGPUs != 0);

    VkPhysicalDevice* foundGPUs = SM::Alloc<VkPhysicalDevice>(numFoundGPUs);
    vkEnumeratePhysicalDevices(m_instance, &numFoundGPUs, foundGPUs);

    if (IsRunningDebugBuild())
    {
        Platform::Log("Physical Devices:\n");

        for (U8 i = 0; i < numFoundGPUs; i++)
        {
            VkPhysicalDeviceProperties deviceProps;
            vkGetPhysicalDeviceProperties(foundGPUs[i], &deviceProps);

            //VkPhysicalDeviceFeatures deviceFeatures;
            //vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

            Platform::Log("%s\n", deviceProps.deviceName);
        }
    }

    for(int iGPU = 0; iGPU < numFoundGPUs; iGPU++)
    {
        const VkPhysicalDevice& candidateGPU = foundGPUs[iGPU];

        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(candidateGPU, &props);

        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures(candidateGPU, &features);

        // must be a dedicated gpu
        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
        {
            continue;
        }

        // must have anisotrophic filtering... I don't remember why tho
        if (features.samplerAnisotropy == VK_FALSE)
        {
            continue;
        }

        U32 numFoundExtensions = 0;
        vkEnumerateDeviceExtensionProperties(candidateGPU, nullptr, &numFoundExtensions, nullptr);

        VkExtensionProperties* extensions = SM::Alloc<VkExtensionProperties>(numFoundExtensions);
        vkEnumerateDeviceExtensionProperties(candidateGPU, nullptr, &numFoundExtensions, extensions);

        U8 numRequiredExtensions = ARRAY_LEN(DEVICE_EXTENSIONS);
        U8 hasExtensionCounter = 0;

        for(U32 i = 0; i < numFoundExtensions; i++)
        {
            const VkExtensionProperties& props = extensions[i];

            for(U32 j = 0; j < numRequiredExtensions; j++)
            {
                const char* extensionName = props.extensionName;
                const char* requiredExtension = DEVICE_EXTENSIONS[j];
                if(strcmp(extensionName, requiredExtension) == 0)
                {
                    hasExtensionCounter++;
                    break;
                }
            }
        }

        // must have all extensions I need
        if(hasExtensionCounter != numRequiredExtensions)
        {
            continue;
        }

        U32 numSurfaceFormats = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(candidateGPU, m_surface, &numSurfaceFormats, nullptr);
        if(numSurfaceFormats == 0)
        {
            continue;
        }

        U32 numPresentModes = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(candidateGPU, m_surface, &numPresentModes, nullptr);
        if(numPresentModes == 0)
        {
            continue;
        }

        U32 numQueueFamilies = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(candidateGPU, &numQueueFamilies, nullptr);

        VkQueueFamilyProperties* queueFamilyProperties = SM::Alloc<VkQueueFamilyProperties>(numQueueFamilies);
        vkGetPhysicalDeviceQueueFamilyProperties(candidateGPU, &numQueueFamilies, queueFamilyProperties);

        m_graphicsQueueIndex = kInvalidQueueIndex;
        m_computeQueueIndex = kInvalidQueueIndex;
        m_transferQueueIndex = kInvalidQueueIndex;
        m_presentationQueueIndex = kInvalidQueueIndex;
        for (int iQueue = 0; iQueue < numQueueFamilies; iQueue++)
        {
            const VkQueueFamilyProperties& props = queueFamilyProperties[iQueue];

            if (m_graphicsQueueIndex == kInvalidQueueIndex && 
                IsBitSet(props.queueFlags, VK_QUEUE_GRAPHICS_BIT))
            {
                m_graphicsQueueIndex = iQueue;
            }

            if (m_computeQueueIndex == kInvalidQueueIndex && 
                IsBitSet(props.queueFlags, VK_QUEUE_COMPUTE_BIT)  && 
                !IsBitSet(props.queueFlags, VK_QUEUE_GRAPHICS_BIT))
            {
                m_computeQueueIndex = iQueue;
            }

            if (m_transferQueueIndex == kInvalidQueueIndex && 
                IsBitSet(props.queueFlags, VK_QUEUE_TRANSFER_BIT) &&
                !IsBitSet(props.queueFlags, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT))
            {
                m_transferQueueIndex = iQueue;
            }

            VkBool32 canPresent = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(candidateGPU, iQueue, m_surface, &canPresent);
            if (m_presentationQueueIndex == kInvalidQueueIndex && canPresent)
            {
                m_presentationQueueIndex = iQueue;
            }

            // check if no more families to find
            if (m_graphicsQueueIndex != kInvalidQueueIndex && 
                m_computeQueueIndex != kInvalidQueueIndex &&
                m_presentationQueueIndex != kInvalidQueueIndex &&
                m_transferQueueIndex != kInvalidQueueIndex)
            {
                break;
            }
        }

        if(m_graphicsQueueIndex == -1 || m_presentationQueueIndex == -1)
        {
            continue;
        }

        m_physicalDevice = candidateGPU;
        vkGetPhysicalDeviceProperties(m_physicalDevice, &m_physicalDeviceProperties);
        vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &m_physicalDeviceMemoryProperties);
    }

    SM_ASSERT(m_physicalDevice != VK_NULL_HANDLE);

    //------------------------------------------------------------------------------------------------------------------------
    // Logical Device
    //------------------------------------------------------------------------------------------------------------------------
    VkPhysicalDeviceFeatures deviceFeatures {
        .fillModeNonSolid = VK_TRUE,
        .samplerAnisotropy = VK_TRUE
    };

    VkPhysicalDeviceVulkan13Features vk13Features {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .synchronization2 = VK_TRUE,
        .dynamicRendering = VK_TRUE
    };

    F32 priority = 1.0f;
    U32 numQueues = 0;

    static const U32 kMaxNumQueues = 4;
    VkDeviceQueueCreateInfo queueCreateInfos[kMaxNumQueues];
    ::memset(queueCreateInfos, 0, sizeof(queueCreateInfos));
    queueCreateInfos[numQueues].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfos[numQueues].queueFamilyIndex = m_graphicsQueueIndex;
    queueCreateInfos[numQueues].queueCount = 1;
    queueCreateInfos[numQueues].pQueuePriorities = &priority;
    numQueues++;

    queueCreateInfos[numQueues].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfos[numQueues].queueFamilyIndex = m_computeQueueIndex;
    queueCreateInfos[numQueues].queueCount = 1;
    queueCreateInfos[numQueues].pQueuePriorities = &priority;
    numQueues++;

    if(m_graphicsQueueIndex != m_presentationQueueIndex)
    {
        queueCreateInfos[numQueues].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfos[numQueues].queueFamilyIndex = m_presentationQueueIndex;
        queueCreateInfos[numQueues].queueCount = 1;
        queueCreateInfos[numQueues].pQueuePriorities = &priority;
        numQueues++;
    }

    if(m_transferQueueIndex != kInvalidQueueIndex)
    {
        queueCreateInfos[numQueues].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfos[numQueues].queueFamilyIndex = m_transferQueueIndex;
        queueCreateInfos[numQueues].queueCount = 1;
        queueCreateInfos[numQueues].pQueuePriorities = &priority;
        numQueues++;
    }

    VkDeviceCreateInfo createInfo {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &vk13Features,
        .queueCreateInfoCount = numQueues,
        .pQueueCreateInfos = queueCreateInfos,
        .enabledExtensionCount = ARRAY_LEN(DEVICE_EXTENSIONS),
        .ppEnabledExtensionNames = DEVICE_EXTENSIONS,
        .pEnabledFeatures = &deviceFeatures
    };

    SM_ASSERT(vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) == VK_SUCCESS);
    Platform::LoadVulkanDeviceFuncs(m_device);

    //------------------------------------------------------------------------------------------------------------------------
    // Queues
    //------------------------------------------------------------------------------------------------------------------------
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue computeQueue = VK_NULL_HANDLE;
    VkQueue presentationQueue = VK_NULL_HANDLE;
    VkQueue transferQueue = VK_NULL_HANDLE;
    {
        vkGetDeviceQueue(m_device, m_graphicsQueueIndex, 0, &m_graphicsQueue);
        vkGetDeviceQueue(m_device, m_computeQueueIndex, 0, &m_computeQueue);
        vkGetDeviceQueue(m_device, m_presentationQueueIndex, 0, &m_presentationQueue);
        if(m_transferQueueIndex != kInvalidQueueIndex)
        {
            vkGetDeviceQueue(m_device, m_transferQueueIndex, 0, &transferQueue);
        }
    }

    //------------------------------------------------------------------------------------------------------------------------
    // Command pools
    //------------------------------------------------------------------------------------------------------------------------
    VkCommandPoolCreateInfo graphicsCommandPoolCreateInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = (U32)m_graphicsQueueIndex
    };
    SM_ASSERT(vkCreateCommandPool(m_device, &graphicsCommandPoolCreateInfo, nullptr, &m_graphicsCommandPool) == VK_SUCCESS);

    //------------------------------------------------------------------------------------------------------------------------
    // Swapchain
    //------------------------------------------------------------------------------------------------------------------------
	if (m_swapchain != VK_NULL_HANDLE)
	{
        vkQueueWaitIdle(m_graphicsQueue);
        vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
	}

    U32 numSurfaceFormats = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &numSurfaceFormats, nullptr);
    VkSurfaceFormatKHR* surfaceFormats = SM::Alloc<VkSurfaceFormatKHR>(numSurfaceFormats);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &numSurfaceFormats, surfaceFormats);

    m_swapchainFormat = surfaceFormats[0];
    for(int i = 0; i < numSurfaceFormats; i++)
    {
        const VkSurfaceFormatKHR& format = surfaceFormats[i];
        if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            m_swapchainFormat = format;
        }
    }

    U32 numPresentModes = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &numPresentModes, nullptr);
    VkPresentModeKHR* presentModes = SM::Alloc<VkPresentModeKHR>(numPresentModes);
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &numPresentModes, presentModes);

    VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    for(int i = 0; i < numPresentModes; i++)
    {
        const VkPresentModeKHR& mode = presentModes[i];
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            swapchainPresentMode = mode;
            break;
        }
    }

    VkSurfaceCapabilitiesKHR surfaceCapabilities{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, &surfaceCapabilities);

    if (surfaceCapabilities.currentExtent.width != UINT32_MAX)
    {
        m_swapchainExtent = surfaceCapabilities.currentExtent;
    }
    else
    {
        U32 width;
        U32 height;
        Platform::GetWindowDimensions(m_pWindow, width, height);
        m_swapchainExtent = { width, height };
        m_swapchainExtent.width = Clamp(m_swapchainExtent.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
        m_swapchainExtent.height = Clamp(m_swapchainExtent.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);
    }

    U32 imageCount = surfaceCapabilities.minImageCount + 1; // one extra image to prevent waiting on driver
    imageCount = Min(imageCount, kMaxNumSwapchainImages);
    if (surfaceCapabilities.maxImageCount > 0)
    {
        imageCount = Min(imageCount, surfaceCapabilities.maxImageCount);
    }

    // TODO: Allow fullscreen
    //VkSurfaceFullScreenExclusiveInfoEXT fullScreenInfo = {};
    //fullScreenInfo.sType = VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_INFO_EXT;
    //fullScreenInfo.pNext = nullptr;
    //fullScreenInfo.fullScreenExclusive = VK_FULL_SCREEN_EXCLUSIVE_ALLOWED_EXT;

    VkSwapchainCreateInfoKHR swapchainCreateInfo {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = nullptr, // use full_screenInfo here
        .surface = m_surface,
        .minImageCount = imageCount,
        .imageFormat = m_swapchainFormat.format,
        .imageColorSpace = m_swapchainFormat.colorSpace,
        .imageExtent = m_swapchainExtent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .presentMode = swapchainPresentMode
    };

    U32 queueFamilyIndices[] = { (U32)m_graphicsQueueIndex, (U32)m_presentationQueueIndex };

    if (m_graphicsQueueIndex != m_presentationQueueIndex)
    {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchainCreateInfo.queueFamilyIndexCount = 2;
        swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainCreateInfo.queueFamilyIndexCount = 0;
        swapchainCreateInfo.pQueueFamilyIndices = nullptr;
    }

    swapchainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCreateInfo.clipped = VK_TRUE;
    swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

    SM_ASSERT(vkCreateSwapchainKHR(m_device, &swapchainCreateInfo, nullptr, &m_swapchain) == VK_SUCCESS);
    SM_ASSERT(vkGetSwapchainImagesKHR(m_device, m_swapchain, &m_numSwapchainImages, nullptr) == VK_SUCCESS);
    SM_ASSERT(m_numSwapchainImages <= kMaxNumSwapchainImages);
    SM_ASSERT(vkGetSwapchainImagesKHR(m_device, m_swapchain, &m_numSwapchainImages, m_swapchainImages) == VK_SUCCESS);

    VkCommandBufferAllocateInfo commandBufferAllocInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = m_graphicsCommandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(m_device, &commandBufferAllocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    // transition swapchain images to presentation layout
    for (U32 i = 0; i < m_numSwapchainImages; i++)
    {
        VkImageMemoryBarrier barrier {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_NONE,
            .dstAccessMask = VK_ACCESS_NONE,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = m_swapchainImages[i],
            .subresourceRange {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };

        vkCmdPipelineBarrier(commandBuffer,
                             VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             0,
                             0, nullptr,
                             0, nullptr,
                             1, &barrier);
    }
    vkEndCommandBuffer(commandBuffer);
    
    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(m_graphicsQueue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_graphicsQueue);

    vkFreeCommandBuffers(m_device, m_graphicsCommandPool, 1, &commandBuffer);

    //------------------------------------------------------------------------------------------------------------------------
    // Random Precalculated Values
    //------------------------------------------------------------------------------------------------------------------------

    // calculate max number of msaa samples we can use
    VkSampleCountFlags msaaCounts = m_physicalDeviceProperties.limits.framebufferColorSampleCounts & m_physicalDeviceProperties.limits.framebufferDepthSampleCounts;
    if (msaaCounts & VK_SAMPLE_COUNT_2_BIT) m_maxMsaaSamples = VK_SAMPLE_COUNT_2_BIT;
    if (msaaCounts & VK_SAMPLE_COUNT_4_BIT) m_maxMsaaSamples = VK_SAMPLE_COUNT_4_BIT;
    if (msaaCounts & VK_SAMPLE_COUNT_8_BIT) m_maxMsaaSamples = VK_SAMPLE_COUNT_8_BIT;
    if (msaaCounts & VK_SAMPLE_COUNT_16_BIT) m_maxMsaaSamples = VK_SAMPLE_COUNT_16_BIT;
    if (msaaCounts & VK_SAMPLE_COUNT_32_BIT) m_maxMsaaSamples = VK_SAMPLE_COUNT_32_BIT;
    if (msaaCounts & VK_SAMPLE_COUNT_64_BIT) m_maxMsaaSamples = VK_SAMPLE_COUNT_64_BIT;

    // default depth format
	VkFormat candidateDepthFormats[] = { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
	m_defaultDepthFormat = FindSupportedFormat(candidateDepthFormats, ARRAY_LEN(candidateDepthFormats), VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	SM_ASSERT(m_defaultDepthFormat != VK_FORMAT_UNDEFINED);

    SM::PopAllocator();

    return true;
}

void VulkanRenderer::RenderFrame()
{
}

VkFormat VulkanRenderer::FindSupportedFormat(VkFormat* candidates, U32 numCandidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	for(U32 i = 0; i < numCandidates; i++)
	{
		const VkFormat& format = candidates[i];

		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(m_physicalDevice, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
		{
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
		{
			return format;
		}
	}
	return VK_FORMAT_UNDEFINED;
}

void VulkanRenderer::SetClearColor(const ColorF32& color)
{
    m_clearColor = color;
}
