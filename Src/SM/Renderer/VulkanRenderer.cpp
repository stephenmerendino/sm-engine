#include "SM/Renderer/VulkanRenderer.h"
#include "SM/Assert.h"
#include "SM/Bits.h"
#include "SM/Memory.h"
#include "SM/Renderer/VulkanConfig.h"
#include "ThirdParty/vulkan/vulkan_core.h"

#include <cstring>

using namespace SM;

bool VulkanRenderer::Init(Platform::Window* pWindow)
{
    //------------------------------------------------------------------------------------------------------------------------
    // Instance
    //------------------------------------------------------------------------------------------------------------------------
    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "SM Workbench",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "SM Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_3
    };

    VkInstanceCreateInfo instanceCreateInfo = {
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

            VkLayerProperties* instanceLayers = SM::Alloc<VkLayerProperties>(kEngineGlobal, numLayers);
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
        VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo = {
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
            VkValidationFeaturesEXT features = {};
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
        VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo = {
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

    VkSurfaceKHR surface = Platform::CreateVulkanSurface(m_instance, pWindow);

    //------------------------------------------------------------------------------------------------------------------------
    // Physical Device
    //------------------------------------------------------------------------------------------------------------------------
    U32 numFoundGPUs = 0;
    vkEnumeratePhysicalDevices(m_instance, &numFoundGPUs, nullptr);
    SM_ASSERT(numFoundGPUs != 0);

    VkPhysicalDevice* foundGPUs = SM::Alloc<VkPhysicalDevice>(kEngineGlobal, numFoundGPUs);
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

    static const I32 kInvalidQueueIndex = -1;
    I32 graphicsQueueIndex = kInvalidQueueIndex;
    I32 computeQueueIndex = kInvalidQueueIndex;
    I32 transferQueueIndex = kInvalidQueueIndex;
    I32 presentationQueueIndex = kInvalidQueueIndex;
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

        VkExtensionProperties* extensions = SM::Alloc<VkExtensionProperties>(kEngineGlobal, numFoundExtensions);
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
        vkGetPhysicalDeviceSurfaceFormatsKHR(candidateGPU, surface, &numSurfaceFormats, nullptr);
        if(numSurfaceFormats == 0)
        {
            continue;
        }

        U32 numPresentModes = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(candidateGPU, surface, &numPresentModes, nullptr);
        if(numPresentModes == 0)
        {
            continue;
        }

        U32 numQueueFamilies = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(candidateGPU, &numQueueFamilies, nullptr);

        VkQueueFamilyProperties* queueFamilyProperties = SM::Alloc<VkQueueFamilyProperties>(kEngineGlobal, numQueueFamilies);
        vkGetPhysicalDeviceQueueFamilyProperties(candidateGPU, &numQueueFamilies, queueFamilyProperties);

        graphicsQueueIndex = kInvalidQueueIndex;
        computeQueueIndex = kInvalidQueueIndex;
        transferQueueIndex = kInvalidQueueIndex;
        presentationQueueIndex = kInvalidQueueIndex;
        for (int iQueue = 0; iQueue < numQueueFamilies; iQueue++)
        {
            const VkQueueFamilyProperties& props = queueFamilyProperties[iQueue];

            if (graphicsQueueIndex == kInvalidQueueIndex && 
                IsBitSet(props.queueFlags, VK_QUEUE_GRAPHICS_BIT))
            {
                graphicsQueueIndex = iQueue;
            }

            if (computeQueueIndex == kInvalidQueueIndex && 
                IsBitSet(props.queueFlags, VK_QUEUE_COMPUTE_BIT)  && 
                !IsBitSet(props.queueFlags, VK_QUEUE_GRAPHICS_BIT))
            {
                computeQueueIndex = iQueue;
            }

            if (transferQueueIndex == kInvalidQueueIndex && 
                IsBitSet(props.queueFlags, VK_QUEUE_TRANSFER_BIT) &&
                !IsBitSet(props.queueFlags, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT))
            {
                transferQueueIndex = iQueue;
            }

            VkBool32 canPresent = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(candidateGPU, iQueue, surface, &canPresent);
            if (presentationQueueIndex == kInvalidQueueIndex && canPresent)
            {
                presentationQueueIndex = iQueue;
            }

            // check if no more families to find
            if (graphicsQueueIndex != kInvalidQueueIndex && 
                computeQueueIndex != kInvalidQueueIndex &&
                presentationQueueIndex != kInvalidQueueIndex &&
                transferQueueIndex != kInvalidQueueIndex)
            {
                break;
            }
        }

        if(graphicsQueueIndex == -1 || presentationQueueIndex == -1)
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
    VkPhysicalDeviceFeatures deviceFeatures = {
        .fillModeNonSolid = VK_TRUE,
        .samplerAnisotropy = VK_TRUE
    };

    VkPhysicalDeviceVulkan13Features vk13Features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .synchronization2 = VK_TRUE,
        .dynamicRendering = VK_TRUE
    };

    F32 priority = 1.0f;
    U32 numQueues = 0;

    static const U32 kMaxNumQueues = 4;
    VkDeviceQueueCreateInfo queueCreateInfos[kMaxNumQueues];
    queueCreateInfos[numQueues].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfos[numQueues].queueFamilyIndex = graphicsQueueIndex;
    queueCreateInfos[numQueues].queueCount = 1;
    queueCreateInfos[numQueues].pQueuePriorities = &priority;
    numQueues++;

    queueCreateInfos[numQueues].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfos[numQueues].queueFamilyIndex = computeQueueIndex;
    queueCreateInfos[numQueues].queueCount = 1;
    queueCreateInfos[numQueues].pQueuePriorities = &priority;
    numQueues++;

    if(graphicsQueueIndex != presentationQueueIndex)
    {
        queueCreateInfos[numQueues].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfos[numQueues].queueFamilyIndex = presentationQueueIndex;
        queueCreateInfos[numQueues].queueCount = 1;
        queueCreateInfos[numQueues].pQueuePriorities = &priority;
        numQueues++;
    }

    if(transferQueueIndex != kInvalidQueueIndex)
    {
        queueCreateInfos[numQueues].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfos[numQueues].queueFamilyIndex = transferQueueIndex;
        queueCreateInfos[numQueues].queueCount = 1;
        queueCreateInfos[numQueues].pQueuePriorities = &priority;
        numQueues++;
    }

    VkDeviceCreateInfo createInfo = {
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
        vkGetDeviceQueue(m_device, graphicsQueueIndex, 0, &m_graphicsQueue);
        vkGetDeviceQueue(m_device, computeQueueIndex, 0, &m_computeQueue);
        vkGetDeviceQueue(m_device, presentationQueueIndex, 0, &m_presentationQueue);
        if(transferQueueIndex != kInvalidQueueIndex)
        {
            vkGetDeviceQueue(m_device, transferQueueIndex, 0, &transferQueue);
        }
    }

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
    VkSurfaceFormatKHR* surfaceFormats = SM::Alloc<VkSurfaceFormatKHR>(s);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &numSurfaceFormats, surfaceFormats);

    VkSurfaceFormatKHR swapchainFormat = surfaceFormats[0];
    for(int i = 0; i < numSurfaceFormats; i++)
    {
        const VkSurfaceFormatKHR& format = surfaceFormats[i];
        if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            swapchainFormat = format;
        }
    }

    U32 numPresentModes = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &numPresentModes, nullptr);
    VkPresentModeKHR* presentModes = SM::Alloc<VkPresentModeKHR>(s);
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

    VkSurfaceCapabilitiesKHR surfaceCapabilities = {};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, &surfaceCapabilities);

    VkExtent2D swapchainExtent{};
    if (surfaceCapabilities.currentExtent.width != UINT32_MAX)
    {
        swapchainExtent = surfaceCapabilities.currentExtent;
    }
    else
    {
        swapchainExtent = { context.window->width, context.window->height };
        swapchainExtent.width = Clamp(swapchainExtent.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
        swapchainExtent.height = Clamp(swapchainExtent.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);
    }

    U32 imageCount = surfaceCapabilities.minImageCount + 1; // one extra image to prevent waiting on driver
    if (surfaceCapabilities.maxImageCount > 0)
    {
        imageCount = Min(imageCount, surfaceCapabilities.maxImageCount);
    }

    // TODO: Allow fullscreen
    //VkSurfaceFullScreenExclusiveInfoEXT fullScreenInfo = {};
    //fullScreenInfo.sType = VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_INFO_EXT;
    //fullScreenInfo.pNext = nullptr;
    //fullScreenInfo.fullScreenExclusive = VK_FULL_SCREEN_EXCLUSIVE_ALLOWED_EXT;

    //VkSwapchainCreateInfoKHR create_info{};
    //create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    //create_info.pNext = nullptr; // use full_screen_info here
    //create_info.surface = context.surface;
    //create_info.minImageCount = image_count;
    //create_info.imageFormat = swapchain_format.format;
    //create_info.imageColorSpace = swapchain_format.colorSpace;
    //create_info.presentMode = swapchain_present_mode;
    //create_info.imageExtent = swapchain_extent;
    //create_info.imageArrayLayers = 1;
    //create_info.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    //u32 queueFamilyIndices[] = { (u32)context.queue_indices.graphics, (u32)context.queue_indices.presentation };

    //if (context.queue_indices.graphics != context.queue_indices.presentation)
    //{
    //    create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    //    create_info.queueFamilyIndexCount = 2;
    //    create_info.pQueueFamilyIndices = queueFamilyIndices;
    //}
    //else
    //{
    //    create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    //    create_info.queueFamilyIndexCount = 0;
    //    create_info.pQueueFamilyIndices = nullptr;
    //}

    //create_info.preTransform = swapchain_info.capabilities.currentTransform;
    //create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    //create_info.clipped = VK_TRUE;
    //create_info.oldSwapchain = VK_NULL_HANDLE;

    //SM_VULKAN_ASSERT(vkCreateSwapchainKHR(context.device, &create_info, nullptr, &context.swapchain.handle));

    //u32 num_images = 0;
    //vkGetSwapchainImagesKHR(context.device, context.swapchain.handle, &num_images, nullptr);

	//array_resize(context.swapchain.images, num_images);
    //vkGetSwapchainImagesKHR(context.device, context.swapchain.handle, &num_images, context.swapchain.images.data);

    //context.swapchain.format = swapchain_format.format;
    //context.swapchain.extent = swapchain_extent;

    //VkCommandBufferAllocateInfo command_buffer_alloc_info{};
    //command_buffer_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    //command_buffer_alloc_info.pNext = nullptr;
    //command_buffer_alloc_info.commandPool = context.graphics_command_pool;
    //command_buffer_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    //command_buffer_alloc_info.commandBufferCount = 1;

    //VkCommandBuffer command_buffer;
    //vkAllocateCommandBuffers(context.device, &command_buffer_alloc_info, &command_buffer);

    //VkCommandBufferBeginInfo begin_info{};
    //begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    //begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    //vkBeginCommandBuffer(command_buffer, &begin_info);

    //// transition swapchain images to presentation layout
    //for (u32 i = 0; i < (u32)context.swapchain.images.cur_size; i++)
    //{
    //    VkImageMemoryBarrier barrier{};
    //    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    //    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    //    barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    //    barrier.srcAccessMask = VK_ACCESS_NONE;
    //    barrier.dstAccessMask = VK_ACCESS_NONE;
    //    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    //    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    //    barrier.image = context.swapchain.images[i];
    //    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    //    barrier.subresourceRange.baseMipLevel = 0;
    //    barrier.subresourceRange.levelCount = 1;
    //    barrier.subresourceRange.baseArrayLayer = 0;
    //    barrier.subresourceRange.layerCount = 1;

    //    vkCmdPipelineBarrier(command_buffer,
    //                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
    //                         0,
    //                         0, nullptr,
    //                         0, nullptr,
    //                         1, &barrier);
    //}
    //vkEndCommandBuffer(command_buffer);
    //
    //VkSubmitInfo submit_info{};
    //submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    //submit_info.commandBufferCount = 1;
    //submit_info.pCommandBuffers = &command_buffer;

    //vkQueueSubmit(context.graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    //vkQueueWaitIdle(context.graphics_queue);

    //vkFreeCommandBuffers(context.device, context.graphics_command_pool, 1, &command_buffer);

    return true;
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
