#include "SM/Renderer/VulkanRenderer.h"
#include "SM/Assert.h"
#include "SM/Bits.h"
#include "SM/Engine.h"
#include "SM/Math.h"
#include "SM/Memory.h"
#include "SM/Renderer/VulkanConfig.h"
#include "SM/Renderer/VulkanFunctions.h"
#include "ThirdParty/vulkan/vulkan_core.h"

#define VK_NO_PROTOTYPES
#include "ThirdParty/imgui/imgui.h"
#include "ThirdParty/imgui/imgui_impl_vulkan.cpp"

#include <cstring>

using namespace SM;

static const ColorF32 kDefaultClearColor = ColorF32(0, 0.4, 0.4);

static void ImguiCheckVulkanResult(VkResult result)
{
    SM_ASSERT(result == VK_SUCCESS);
}

static PFN_vkVoidFunction ImguiVulkanFuncLoader(const char* functionName, void* userData)
{
	VkInstance instance = *((VkInstance*)userData);
    return vkGetInstanceProcAddr(instance, functionName);
}

void FrameResources::Init(VulkanRenderer* pRenderer)
{
    m_pRenderer = pRenderer;
    m_curScreenResolution = {.width = 0, .height = 0};
    
    // m_commandBuffer
    {
        VkCommandBufferAllocateInfo commandBufferAllocInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .pNext = nullptr,
                .commandPool = m_pRenderer->m_graphicsCommandPool,
                .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount = 1 
        };

        vkAllocateCommandBuffers(m_pRenderer->m_device, &commandBufferAllocInfo, &m_commandBuffer);
    }

    // m_swapchainImageAcquiredSemaphore
    {
        VkSemaphoreCreateInfo createInfo{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0 
        };
        SM_ASSERT(vkCreateSemaphore(m_pRenderer->m_device, &createInfo, nullptr, &m_swapchainImageAcquiredSemaphore) == VK_SUCCESS);
    }

    // m_swapchainImageAcquiredFence
    {
        VkFenceCreateInfo createInfo{
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
        };
        SM_ASSERT(vkCreateFence(m_pRenderer->m_device, &createInfo, nullptr, &m_swapchainImageAcquiredFence) == VK_SUCCESS);
    }

    // m_frameCompletedFence
    {
        VkFenceCreateInfo createInfo{
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
        };
        SM_ASSERT(vkCreateFence(m_pRenderer->m_device, &createInfo, nullptr, &m_frameCompletedFence) == VK_SUCCESS);
    }

    // m_allGpuWorkCompletedSemaphore
    {
        VkSemaphoreCreateInfo createInfo{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0 
        };
        SM_ASSERT(vkCreateSemaphore(m_pRenderer->m_device, &createInfo, nullptr, &m_allGpuWorkCompletedSemaphore) == VK_SUCCESS);
    }
}

void FrameResources::BeginFrame()
{
    // block main thread to explicitly wait and reset of the main frame fence, this means all previous gpu work is finished
    {
        VkFence fencesToReset[] = {
            m_frameCompletedFence,
            m_swapchainImageAcquiredFence
        };
        vkWaitForFences(m_pRenderer->m_device, ARRAY_LEN(fencesToReset), fencesToReset, VK_TRUE, UINT64_MAX);
        vkResetFences(m_pRenderer->m_device, ARRAY_LEN(fencesToReset), fencesToReset);
    }

    // swapchain update
    {
        bool bSwapchainStillUsable = m_pRenderer->UpdateSwapchain(m_swapchainImageIndex, m_swapchainImageAcquiredSemaphore, m_swapchainImageAcquiredFence);
        SM_ASSERT(bSwapchainStillUsable);
    }

    // reset and begin the frame command buffer
    {
        VkCommandBufferResetFlags resetCommandBufferFlags = 0;
        vkResetCommandBuffer(m_commandBuffer, resetCommandBufferFlags);

        VkCommandBufferBeginInfo beginInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = nullptr,
            .flags = 0,
            .pInheritanceInfo = nullptr
        };
        SM_ASSERT(vkBeginCommandBuffer(m_commandBuffer, &beginInfo) == VK_SUCCESS);
    }

    // swapchain res changed so we have to update all of our resources to match the new size
    if(m_curScreenResolution != m_pRenderer->m_swapchainExtent)
    {
        // free previous resources
        if(m_curScreenResolution.width != 0 && m_curScreenResolution.height != 0)
        {
            vkDestroyImage(m_pRenderer->m_device, m_mainColorRenderTarget, nullptr);
            vkFreeMemory(m_pRenderer->m_device, m_mainColorRenderTargetMemory, nullptr);
        }

        m_curScreenResolution = m_pRenderer->m_swapchainExtent;

        // setup main color render target
        {
            m_mainColorExtent = m_curScreenResolution;

            // Image
            VkImageCreateInfo imageCreateInfo {
                .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, 
                    .pNext = nullptr, 
                    .flags = 0, 
                    .imageType = VK_IMAGE_TYPE_2D, 
                    .format = VulkanRenderer::kMainColorFormat, 
                    .extent = { .width = m_mainColorExtent.width, .height = m_mainColorExtent.height, .depth = 1 }, 
                    .mipLevels = 1, 
                    .arrayLayers = 1,
                    .samples = VK_SAMPLE_COUNT_1_BIT,
                    .tiling = VK_IMAGE_TILING_OPTIMAL, 
                    .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, 
                    .sharingMode = VK_SHARING_MODE_EXCLUSIVE, 
                    .queueFamilyIndexCount = 1, 
                    .pQueueFamilyIndices = (U32*)&m_pRenderer->m_graphicsQueueIndex, 
                    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED 
            };

            SM_ASSERT(vkCreateImage(m_pRenderer->m_device, &imageCreateInfo, nullptr, &m_mainColorRenderTarget) == VK_SUCCESS);

            // Memory
            VkMemoryRequirements imageMemoryRequirements;
            vkGetImageMemoryRequirements(m_pRenderer->m_device, m_mainColorRenderTarget, &imageMemoryRequirements);

            VkMemoryPropertyFlagBits neededFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            U32 memoryTypeIndex = UINT32_MAX;

            // for each memory type check if its bit is set, if so check if it has the flags we need
            for(int i = 0; i < m_pRenderer->m_physicalDeviceMemoryProperties.memoryTypeCount; i++)
            {
                // not a valid memory type based on the requirements
                if(!IsBitSet(imageMemoryRequirements.memoryTypeBits, i))
                {
                    continue;
                }

                // its a valid memory type, BUT it doesn't have the flags we need
                if(!IsBitSet(m_pRenderer->m_physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags, neededFlags))
                {
                    continue;    
                }

                // passed all the filters, this is a valid memory type to use
                memoryTypeIndex = i;
                break;
            }

            VkMemoryAllocateInfo imageMemoryAllocateInfo {
                .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, 
                    .pNext = nullptr,
                    .allocationSize = imageMemoryRequirements.size,
                    .memoryTypeIndex = memoryTypeIndex
            };

            SM_ASSERT(vkAllocateMemory(m_pRenderer->m_device, &imageMemoryAllocateInfo, nullptr, &m_mainColorRenderTargetMemory) == VK_SUCCESS);

            VkDeviceSize memoryOffset = 0;
            SM_ASSERT(vkBindImageMemory(m_pRenderer->m_device, m_mainColorRenderTarget, m_mainColorRenderTargetMemory, memoryOffset) == VK_SUCCESS);

            // Image View
            VkImageViewCreateInfo imageViewCreateInfo{
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .image = m_mainColorRenderTarget,
                    .viewType = VK_IMAGE_VIEW_TYPE_2D,
                    .format = VulkanRenderer::kMainColorFormat,
                    .components = { 
                        .r = VK_COMPONENT_SWIZZLE_IDENTITY, 
                        .g = VK_COMPONENT_SWIZZLE_IDENTITY, 
                        .b = VK_COMPONENT_SWIZZLE_IDENTITY, 
                        .a = VK_COMPONENT_SWIZZLE_IDENTITY 
                    },
                    .subresourceRange = {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel = 0,
                        .levelCount = 1,
                        .baseArrayLayer = 0,
                        .layerCount = 1
                    }
            };
            SM_ASSERT(vkCreateImageView(m_pRenderer->m_device, &imageViewCreateInfo, nullptr, &m_mainColorImageView) == VK_SUCCESS);

            // transition the newly created image into the expected layout for rendering a frame
            VkImageMemoryBarrier barrier {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .srcAccessMask = VK_ACCESS_NONE,
                .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = m_mainColorRenderTarget,
                .subresourceRange {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1
                }
            };
            vkCmdPipelineBarrier(m_commandBuffer,
                                 VK_PIPELINE_STAGE_NONE, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);
        }
    }
}

void FrameResources::EndFrame()
{
    vkEndCommandBuffer(m_commandBuffer);
}

void VulkanRenderer::CreateSwapchain()
{
    PushScopedStackAllocator(KiB(1));

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

    U32 imageCount = Clamp(VulkanConfig::kOptimalNumFramesInFlight, 
                           surfaceCapabilities.minImageCount, 
                           surfaceCapabilities.maxImageCount > 0 ? surfaceCapabilities.maxImageCount : VulkanConfig::kOptimalNumFramesInFlight);

    if(m_numFramesInFlight != imageCount)
    {
        m_numFramesInFlight = imageCount;
        m_pSwapchainImages = SM::Alloc<VkImage>(kEngineGlobal, m_numFramesInFlight);
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
        .minImageCount = m_numFramesInFlight,
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

    // make sure number of swapchain images matches what we expected based on surface capabilities + config
    U32 numSwapchainImages = 0;
    SM_ASSERT(vkGetSwapchainImagesKHR(m_device, m_swapchain, &numSwapchainImages, nullptr) == VK_SUCCESS);
    SM_ASSERT(m_numFramesInFlight == numSwapchainImages);
    SM_ASSERT(vkGetSwapchainImagesKHR(m_device, m_swapchain, &numSwapchainImages, m_pSwapchainImages) == VK_SUCCESS);

    VkCommandBufferAllocateInfo commandBufferAllocInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr, 
        .commandPool = m_graphicsCommandPool, 
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY, 
        .commandBufferCount = 1
    };

    VkCommandBuffer initCommandBuffer = VK_NULL_HANDLE;
    SM_ASSERT(vkAllocateCommandBuffers(m_device, &commandBufferAllocInfo, &initCommandBuffer) == VK_SUCCESS);

    VkCommandBufferBeginInfo beginInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    vkBeginCommandBuffer(initCommandBuffer, &beginInfo);

    // transition swapchain images to presentation layout
    for (U32 i = 0; i < m_numFramesInFlight; i++)
    {
        VkImageMemoryBarrier barrier {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_NONE,
            .dstAccessMask = VK_ACCESS_NONE,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = m_pSwapchainImages[i],
            .subresourceRange {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };

        vkCmdPipelineBarrier(initCommandBuffer,
                             VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             0,
                             0, nullptr,
                             0, nullptr,
                             1, &barrier);
    }
    vkEndCommandBuffer(initCommandBuffer);
    
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &initCommandBuffer;

    vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_graphicsQueue);

    SM::PopAllocator();
}

bool VulkanRenderer::UpdateSwapchain(U32& outSwapchainImageIndex, VkSemaphore imageAcquiredSemaphore, VkFence imageAcquiredFence)
{
    SM_ASSERT(m_swapchain != VK_NULL_HANDLE);

    VkResult status = vkAcquireNextImageKHR(m_device, 
                                            m_swapchain, 
                                            UINT64_MAX, 
                                            imageAcquiredSemaphore, 
                                            imageAcquiredFence, 
                                            &outSwapchainImageIndex);

    SM_ASSERT(status == VK_SUCCESS || status == VK_SUBOPTIMAL_KHR);
    if(status == VK_SUBOPTIMAL_KHR)
    {
        m_bSwapchainNeedsRefresh = true;
    }

    return true;
}

bool VulkanRenderer::Init(Platform::Window* pWindow)
{
    SM::PushAllocator(kEngineGlobal);

    m_pWindow = pWindow;
    m_clearColor = kDefaultClearColor;

    Platform::LoadVulkanGlobalFuncs();

    //------------------------------------------------------------------------------------------------------------------------
    // Instance
    //------------------------------------------------------------------------------------------------------------------------
    VkApplicationInfo appInfo{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "SM Workbench",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "SM Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_3
    };

    VkInstanceCreateInfo instanceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
        .enabledExtensionCount = ARRAY_LEN(VulkanConfig::kInstanceExtensions),
        .ppEnabledExtensionNames = VulkanConfig::kInstanceExtensions
    };

    if (VulkanConfig::kEnableValidationLayers)
    {
        // check validation layers are supported
        {
            U32 numLayers;
            vkEnumerateInstanceLayerProperties(&numLayers, nullptr);

            VkLayerProperties* instanceLayers = SM::Alloc<VkLayerProperties>(numLayers);
            vkEnumerateInstanceLayerProperties(&numLayers, instanceLayers);

            for (const char* layerName : VulkanConfig::kValidationLayers)
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

        instanceCreateInfo.ppEnabledLayerNames = VulkanConfig::kValidationLayers;
        instanceCreateInfo.enabledLayerCount = ARRAY_LEN(VulkanConfig::kValidationLayers);

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

        if (VulkanConfig::kEnableValidationBestPractices)
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

    // immediately load imgui functions once we have a valid instance, otherwise we crash because vulkan functions aren't setup correctly
    {
        ImGui_ImplVulkan_LoadFunctions(VK_API_VERSION_1_3, ImguiVulkanFuncLoader, &m_instance);
    }

    //------------------------------------------------------------------------------------------------------------------------
    // Debug Messenger
    //------------------------------------------------------------------------------------------------------------------------
    if (VulkanConfig::kEnableValidationLayers)
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
    Platform::Log("%lu", (uintptr_t)vkEnumeratePhysicalDevices);
    vkEnumeratePhysicalDevices(m_instance, &numFoundGPUs, nullptr);
    SM_ASSERT(numFoundGPUs != 0);

    VkPhysicalDevice* foundGPUs = SM::Alloc<VkPhysicalDevice>(numFoundGPUs);
    vkEnumeratePhysicalDevices(m_instance, &numFoundGPUs, foundGPUs);

    if(IsRunningDebugBuild())
    {
        Platform::Log("Physical Devices:\n");

        for(U8 i = 0; i < numFoundGPUs; i++)
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
        if(props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
        {
            continue;
        }

        // must have anisotrophic filtering... I don't remember why tho
        if(features.samplerAnisotropy == VK_FALSE)
        {
            continue;
        }

        U32 numFoundExtensions = 0;
        vkEnumerateDeviceExtensionProperties(candidateGPU, nullptr, &numFoundExtensions, nullptr);

        VkExtensionProperties* extensions = SM::Alloc<VkExtensionProperties>(numFoundExtensions);
        vkEnumerateDeviceExtensionProperties(candidateGPU, nullptr, &numFoundExtensions, extensions);

        U8 numRequiredExtensions = ARRAY_LEN(VulkanConfig::kDeviceExtensions);
        U8 hasExtensionCounter = 0;

        for(U32 i = 0; i < numFoundExtensions; i++)
        {
            const VkExtensionProperties& props = extensions[i];

            for(U32 j = 0; j < numRequiredExtensions; j++)
            {
                const char* extensionName = props.extensionName;
                const char* requiredExtension = VulkanConfig::kDeviceExtensions[j];
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
        for(int iQueue = 0; iQueue < numQueueFamilies; iQueue++)
        {
            const VkQueueFamilyProperties& props = queueFamilyProperties[iQueue];

            if(m_graphicsQueueIndex == kInvalidQueueIndex && 
                IsBitSet(props.queueFlags, VK_QUEUE_GRAPHICS_BIT))
            {
                m_graphicsQueueIndex = iQueue;
            }

            if(m_computeQueueIndex == kInvalidQueueIndex && 
                IsBitSet(props.queueFlags, VK_QUEUE_COMPUTE_BIT)  && 
                !IsBitSet(props.queueFlags, VK_QUEUE_GRAPHICS_BIT))
            {
                m_computeQueueIndex = iQueue;
            }

            if(m_transferQueueIndex == kInvalidQueueIndex && 
                IsBitSet(props.queueFlags, VK_QUEUE_TRANSFER_BIT) &&
                !IsBitSet(props.queueFlags, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT))
            {
                m_transferQueueIndex = iQueue;
            }

            VkBool32 canPresent = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(candidateGPU, iQueue, m_surface, &canPresent);
            if(m_presentationQueueIndex == kInvalidQueueIndex && canPresent)
            {
                m_presentationQueueIndex = iQueue;
            }

            // check if no more families to find
            if(m_graphicsQueueIndex != kInvalidQueueIndex && 
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

        if(IsRunningDebugBuild())
        {
            // print out each heap
            for(int i = 0; i < m_physicalDeviceMemoryProperties.memoryHeapCount; i++)
            {
                Platform::Log("[vk-init] Memory Heap %i\n  size = %lu\n  size KiB = %.2f\n  size MiB = %.2f\n  size GiB = %.2f\n  device local = %s\n  multi instance = %s\n", 
                              i,
                              m_physicalDeviceMemoryProperties.memoryHeaps[i].size, 
                              (F32)m_physicalDeviceMemoryProperties.memoryHeaps[i].size / (F32)(1024), 
                              (F32)m_physicalDeviceMemoryProperties.memoryHeaps[i].size / (F32)(1024 * 1024), 
                              (F32)m_physicalDeviceMemoryProperties.memoryHeaps[i].size / (F32)(1024 * 1024 * 1024), 
                              ToString(m_physicalDeviceMemoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT),
                              ToString(m_physicalDeviceMemoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_MULTI_INSTANCE_BIT));
            }

            // print out each memory type
            for(int i = 0; i < m_physicalDeviceMemoryProperties.memoryTypeCount; i++)
            {
                Platform::Log("[vk-init] Memory Type %i\n  heap index = %u\n  device local = %s\n  host visible = %s\n  host coherhent = %s\n  host cached = %s\n  lazily allocated = %s\n", 
                              i,
                              m_physicalDeviceMemoryProperties.memoryTypes[i].heapIndex, 
                              ToString(m_physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
                              ToString(m_physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT),
                              ToString(m_physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
                              ToString(m_physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT),
                              ToString(m_physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT));
            }
        }
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
        .enabledExtensionCount = ARRAY_LEN(VulkanConfig::kDeviceExtensions),
        .ppEnabledExtensionNames = VulkanConfig::kDeviceExtensions,
        .pEnabledFeatures = &deviceFeatures
    };

    SM_VULKAN_ASSERT(vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device));
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
    // Misc
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

    // Setup frame resources
    CreateSwapchain();

    for(int i = 0; i < m_numFramesInFlight; i++)
    {
        m_frameResources[i].Init(this);
    }

    //------------------------------------------------------------------------------------------------------------------------
    // ImGui
    //------------------------------------------------------------------------------------------------------------------------
    {
		// imgui descriptor pool
		{
            const I32 IMGUI_MAX_SETS = 1000;

            VkDescriptorPoolSize poolSizes[] = { 
				{ VK_DESCRIPTOR_TYPE_SAMPLER, IMGUI_MAX_SETS },
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, IMGUI_MAX_SETS },
				{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, IMGUI_MAX_SETS },
				{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, IMGUI_MAX_SETS },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, IMGUI_MAX_SETS },
				{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, IMGUI_MAX_SETS },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, IMGUI_MAX_SETS },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, IMGUI_MAX_SETS },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, IMGUI_MAX_SETS },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, IMGUI_MAX_SETS },
				{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, IMGUI_MAX_SETS }
			};

            VkDescriptorPoolCreateInfo createInfo{
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                .maxSets = IMGUI_MAX_SETS,
                .poolSizeCount = (U32)ARRAY_LEN(poolSizes),
                .pPoolSizes = poolSizes
            };
            SM_VULKAN_ASSERT(vkCreateDescriptorPool(m_device, &createInfo, nullptr, &m_imguiDescriptorPool));
		}

        IMGUI_CHECKVERSION();
		ImGui::CreateContext();
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        ImGui::StyleColorsDark();

        F32 fontSize = 13.0f;
        Platform::ImguiInit(m_pWindow, fontSize);

        VkPipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
            .pNext = nullptr,
            .viewMask = 0,
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &VulkanRenderer::kMainColorFormat,
            .depthAttachmentFormat = VK_FORMAT_UNDEFINED,
            .stencilAttachmentFormat = VK_FORMAT_UNDEFINED
        };

        ImGui_ImplVulkan_InitInfo imguiInitInfo{
            .Instance = m_instance,
            .PhysicalDevice = m_physicalDevice,
            .Device = m_device,
            .QueueFamily = (U32)m_graphicsQueueIndex,
            .Queue = m_graphicsQueue,
            .DescriptorPool = m_imguiDescriptorPool,
            .RenderPass = VK_NULL_HANDLE,
            .MinImageCount = (U32)m_numFramesInFlight, 
            .ImageCount = (U32)m_numFramesInFlight,
            .MSAASamples = VK_SAMPLE_COUNT_1_BIT,
            .PipelineCache = VK_NULL_HANDLE,
            .Subpass = 0,
            .UseDynamicRendering = true,
            .PipelineRenderingCreateInfo = pipelineRenderingCreateInfo,
            .Allocator = VK_NULL_HANDLE,
            .CheckVkResultFn = ImguiCheckVulkanResult
        };
        ImGui_ImplVulkan_Init(&imguiInitInfo);
        ImGui_ImplVulkan_CreateFontsTexture();
	}

    SM::PopAllocator();

    return true;
}

void VulkanRenderer::RenderFrame()
{
    if(ExitRequested() || Platform::IsWindowMinimized(m_pWindow))
    {
        return;    
    }

    m_curFrameInFlight++;
    m_curFrameInFlight %= m_numFramesInFlight;

    // setup new frame
    FrameResources& frameResources = m_frameResources[m_curFrameInFlight];
    frameResources.BeginFrame();

    // imgui
    {
        Platform::ImguiBeginFrame();
        ::ImGui_ImplVulkan_NewFrame();
        ::ImGui::NewFrame();
    }

    // draw to main color target
    {
        VkRenderingAttachmentInfo mainColorAttachmentInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .pNext = nullptr,
            .imageView = frameResources.m_mainColorImageView,
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .resolveMode = VK_RESOLVE_MODE_NONE,
            .resolveImageView = VK_NULL_HANDLE,
            .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue {
                .color {
                    m_clearColor.r,
                    m_clearColor.g,
                    m_clearColor.b,
                    m_clearColor.a
                } 
            }
        };

        VkRenderingAttachmentInfo colorAttachments[] = { mainColorAttachmentInfo };

        VkRenderingInfo renderingInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .pNext = nullptr,
            .flags = 0,
            .renderArea = {
                .offset = { .x = 0, .y = 0 },
                .extent = frameResources.m_mainColorExtent
            },
            .layerCount = 1,
            .viewMask = 0,
            .colorAttachmentCount = ARRAY_LEN(colorAttachments),
            .pColorAttachments = colorAttachments,
            .pDepthAttachment = nullptr,
            .pStencilAttachment = nullptr
        };
        vkCmdBeginRendering(frameResources.m_commandBuffer, &renderingInfo);
        //(TODO) do the rendering of meshes here
        vkCmdEndRendering(frameResources.m_commandBuffer);
    }

    // imgui
    {
        static bool s_showImguiDemo = true;

        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::MenuItem("ImGui Demo"))
            {
                s_showImguiDemo = true;
            }
            ImGui::EndMainMenuBar();
        }

        ImGui::ShowDemoWindow(&s_showImguiDemo);

        VkRenderingAttachmentInfo colorAttachmentInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .pNext = nullptr,
            .imageView = frameResources.m_mainColorImageView,
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .resolveMode = VK_RESOLVE_MODE_NONE,
            .resolveImageView = VK_NULL_HANDLE,
            .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue {
                .color { 0.0f, 0.0f, 0.0f, 0.0f } 
            }
        };
        VkRenderingAttachmentInfo colorAttachments[] = {
            colorAttachmentInfo
        };

        VkRect2D renderArea{
            .offset = { .x = 0, .y = 0 },
            .extent = frameResources.m_mainColorExtent 
        };

        VkRenderingInfo renderingInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .pNext = nullptr,
            .flags = 0,
            .renderArea = renderArea,
            .layerCount = 1,
            .viewMask = 0,
            .colorAttachmentCount = ARRAY_LEN(colorAttachments),
            .pColorAttachments = colorAttachments,
            .pDepthAttachment = nullptr,
            .pStencilAttachment = nullptr 
        };
        vkCmdBeginRendering(frameResources.m_commandBuffer, &renderingInfo);

        ::ImGui::Render();
        ::ImDrawData* draw_data = ::ImGui::GetDrawData();
        ImGui_ImplVulkan_RenderDrawData(draw_data, frameResources.m_commandBuffer);

        vkCmdEndRendering(frameResources.m_commandBuffer);
    }

    // transition main color from color attachment to transfer src
    // transition swapchain image from presentation to transfer dst
    {
        VkImageMemoryBarrier transitionMainColorToTransferSrcBarrier{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = frameResources.m_mainColorRenderTarget,
            .subresourceRange{
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };

        VkImageMemoryBarrier transitionSwapchainToTransferDstBarrier{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = VK_ACCESS_NONE,
            .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = m_pSwapchainImages[frameResources.m_swapchainImageIndex],
            .subresourceRange{
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };

        VkImageMemoryBarrier imageMemoryBarriers[] = {
            transitionMainColorToTransferSrcBarrier,
            transitionSwapchainToTransferDstBarrier
        };

        VkDependencyFlagBits test;
        vkCmdPipelineBarrier(frameResources.m_commandBuffer, 
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 
                VK_PIPELINE_STAGE_TRANSFER_BIT, 
                0, 
                0, nullptr, 
                0, nullptr, 
                ARRAY_LEN(imageMemoryBarriers), imageMemoryBarriers);
    }

    // copy main color data to the swapchain image
    {
        VkImageBlit blitInfo{
            .srcSubresource{
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
            .srcOffsets{
                {.x = 0, .y = 0, .z = 0}, 
                {.x = (int)frameResources.m_mainColorExtent.width, .y = (int)frameResources.m_mainColorExtent.height, .z = 1}
            },
            .dstSubresource{
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
            .dstOffsets{
                {.x = 0, .y = 0, .z = 0}, 
                {.x = (int)m_swapchainExtent.width, .y = (int)m_swapchainExtent.height, .z = 1}
            }
        };
        vkCmdBlitImage(frameResources.m_commandBuffer, 
                frameResources.m_mainColorRenderTarget, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
                m_pSwapchainImages[frameResources.m_swapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
                1, &blitInfo, VK_FILTER_NEAREST);
    }

    // transition swapchain image to presentation from transfer dst
    // transition main color render target to color attachment from transfer src
    {
        VkImageMemoryBarrier transitionMainColorToColorAttachmentBarrier{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
            .dstAccessMask = VK_ACCESS_NONE,
            .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = frameResources.m_mainColorRenderTarget,
            .subresourceRange{
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };

        VkImageMemoryBarrier transitionSwapchainToPresentationBarrier{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_NONE,
            .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = m_pSwapchainImages[frameResources.m_swapchainImageIndex],
            .subresourceRange{
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };

        VkImageMemoryBarrier imageMemoryBarriers[] = {
            transitionMainColorToColorAttachmentBarrier,
            transitionSwapchainToPresentationBarrier
        };

        VkDependencyFlagBits test;
        vkCmdPipelineBarrier(frameResources.m_commandBuffer, 
                VK_PIPELINE_STAGE_TRANSFER_BIT, 
                VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 
                0, 
                0, nullptr, 
                0, nullptr, 
                ARRAY_LEN(imageMemoryBarriers), imageMemoryBarriers);
    }

    frameResources.EndFrame();

    // submit the command buffer
    {
        VkPipelineStageFlags pWaitDstStageMask[]{
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT
        };
        VkSubmitInfo submitInfo{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = nullptr,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &frameResources.m_swapchainImageAcquiredSemaphore,
            .pWaitDstStageMask = pWaitDstStageMask,
            .commandBufferCount = 1,
            .pCommandBuffers = &frameResources.m_commandBuffer,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &frameResources.m_allGpuWorkCompletedSemaphore

        };
        vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, frameResources.m_frameCompletedFence);
    }

    // present the swapchain image
    {
        VkResult presentResult;
        VkPresentInfoKHR presentInfo{
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .pNext = nullptr,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &frameResources.m_allGpuWorkCompletedSemaphore,
            .swapchainCount = 1,
            .pSwapchains = &m_swapchain,
            .pImageIndices = &frameResources.m_swapchainImageIndex,
            .pResults = &presentResult
        };
        presentResult = vkQueuePresentKHR(m_graphicsQueue, &presentInfo);
        SM_ASSERT(presentResult == VK_SUCCESS || presentResult == VK_SUBOPTIMAL_KHR || presentResult == VK_ERROR_OUT_OF_DATE_KHR);
        if(presentResult == VK_SUBOPTIMAL_KHR || presentResult == VK_ERROR_OUT_OF_DATE_KHR)
        {
            m_bSwapchainNeedsRefresh = true;
        }
    }

    // wait until very end of the frame to recreate swapchain if needed
    if(m_bSwapchainNeedsRefresh)
    {
        vkQueueWaitIdle(m_graphicsQueue);
        vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
        CreateSwapchain();
        m_bSwapchainNeedsRefresh = false;
    }
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
