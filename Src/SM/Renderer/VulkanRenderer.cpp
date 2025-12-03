#include "SM/Renderer/VulkanRenderer.h"
#include "SM/Memory.h"

using namespace SM;

//-----------------------------------------------------------------------------------------------------
// Vulkan
#define VK_NO_PROTOTYPES
#include "ThirdParty/vulkan/vulkan.h"
#include "ThirdParty/vulkan/vulkan_win32.h"
#define VK_EXPORTED_FUNCTION(func)	PFN_##func func = VK_NULL_HANDLE;
#define VK_GLOBAL_FUNCTION(func)	PFN_##func func = VK_NULL_HANDLE;
#define VK_INSTANCE_FUNCTION(func)	PFN_##func func = VK_NULL_HANDLE;
#define VK_DEVICE_FUNCTION(func)	PFN_##func func = VK_NULL_HANDLE;
#include "SM/Renderer/VulkanFunctionsManifest.inl"

#if defined(NDEBUG)
	static const char* INSTANCE_EXTENSIONS[] = {
		"VK_KHR_surface",
		"VK_KHR_win32_surface"
	};

	static const char* DEVICE_EXTENSIONS[] = {
		"VK_KHR_swapchain",
        "VK_KHR_dynamic_rendering"
	};

	static const bool VULKAN_VERBOSE = false;
	static const bool ENABLE_VALIDATION_LAYERS = false;
	static const bool ENABLE_VALIDATION_BEST_PRACTICES = false;
#else
	static const char* INSTANCE_EXTENSIONS[] = {
		"VK_KHR_surface",
		"VK_KHR_win32_surface",
		"VK_EXT_debug_utils",
		"VK_EXT_validation_features",
	};

	static const char* DEVICE_EXTENSIONS[] = {
		"VK_KHR_swapchain",
        "VK_KHR_dynamic_rendering"
	};

	static const bool VULKAN_VERBOSE = true;
	static const bool ENABLE_VALIDATION_LAYERS = true;
	static const bool ENABLE_VALIDATION_BEST_PRACTICES = false;
#endif

static const char* VALIDATION_LAYERS[] = {
	"VK_LAYER_KHRONOS_validation"
};
//-----------------------------------------------------------------------------------------------------

bool VulkanRenderer::Init()
{
    #define VK_EXPORTED_FUNCTION(func) \
        HMODULE vulkan_lib = ::LoadLibraryA("vulkan-1.dll"); \
        SM_ASSERT(nullptr != vulkan_lib); \
        func = (PFN_##func)::GetProcAddress(vulkan_lib, #func); \
        SM_ASSERT(nullptr != func);

    #define VK_GLOBAL_FUNCTION(func) \
        func = (PFN_##func)vkGetInstanceProcAddr(nullptr, #func); \
        SM_ASSERT(nullptr != func);

    #include "SM/Renderer/VulkanFunctionsManifest.inl"

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

            .pfnUserCallback = VulkanDebugCallback,
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

    VkInstance instance = VK_NULL_HANDLE;
    SM_ASSERT(vkCreateInstance(&instanceCreateInfo, nullptr, &instance) == VK_SUCCESS);

    // load instance funcs
    #define VK_INSTANCE_FUNCTION(func) \
        func = (PFN_##func)vkGetInstanceProcAddr(instance, #func); \
        SM_ASSERT(nullptr != func);

    #include "SM/VulkanFunctionsManifest.inl"

    // real debug messenger for the whole game
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

            .pfnUserCallback = VulkanDebugCallback,
            .pUserData = nullptr
        };
        VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
        SM_ASSERT(vkCreateDebugUtilsMessengerEXT(instance, &debugMessengerCreateInfo, nullptr, &debugMessenger) == VK_SUCCESS);
    }

    // vk surface
    VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .pNext = nullptr,
        .hinstance = ::GetModuleHandle(nullptr),
        .hwnd = window.m_hwnd
    };
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    SM_ASSERT(vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo, nullptr, &surface) == VK_SUCCESS);

    // vk device
    VkPhysicalDevice selectedGPU = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties gpuProperties = {};
    VkPhysicalDeviceMemoryProperties gpuMemoryProperties = {};

    static const I32 kInvalidQueueIndex = -1;
    I32 graphicsQueueIndex = kInvalidQueueIndex;
    I32 computeQueueIndex = kInvalidQueueIndex;
    I32 transferQueueIndex = kInvalidQueueIndex;
    I32 presentationQueueIndex = kInvalidQueueIndex;

    U32 numFoundGPUs = 0;
    vkEnumeratePhysicalDevices(instance, &numFoundGPUs, nullptr);
    SM_ASSERT(numFoundGPUs != 0);

    VkPhysicalDevice* foundGPUs = SM::Alloc<VkPhysicalDevice>(kEngineGlobal, numFoundGPUs);
    vkEnumeratePhysicalDevices(instance, &numFoundGPUs, foundGPUs);

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

        selectedGPU = candidateGPU;
        vkGetPhysicalDeviceProperties(selectedGPU, &gpuProperties);
        vkGetPhysicalDeviceMemoryProperties(selectedGPU, &gpuMemoryProperties);
    }

    SM_ASSERT(selectedGPU != VK_NULL_HANDLE);

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

    VkDevice device = VK_NULL_HANDLE;
    SM_ASSERT(vkCreateDevice(selectedGPU, &createInfo, nullptr, &device) == VK_SUCCESS);

    // load device funcs
    #define VK_DEVICE_FUNCTION(func) \
        func = (PFN_##func)vkGetDeviceProcAddr(device, #func); \
        SM_ASSERT(nullptr != func);

    #include "SM/VulkanFunctionsManifest.inl"

    // calculate max number of msaa samples we can use
    VkSampleCountFlagBits maxMsaaSamples = VK_SAMPLE_COUNT_1_BIT;
    VkSampleCountFlags msaaCounts = gpuProperties.limits.framebufferColorSampleCounts & gpuProperties.limits.framebufferDepthSampleCounts;
    if (msaaCounts & VK_SAMPLE_COUNT_2_BIT) maxMsaaSamples = VK_SAMPLE_COUNT_2_BIT;
    if (msaaCounts & VK_SAMPLE_COUNT_4_BIT) maxMsaaSamples = VK_SAMPLE_COUNT_4_BIT;
    if (msaaCounts & VK_SAMPLE_COUNT_8_BIT) maxMsaaSamples = VK_SAMPLE_COUNT_8_BIT;
    if (msaaCounts & VK_SAMPLE_COUNT_16_BIT) maxMsaaSamples = VK_SAMPLE_COUNT_16_BIT;
    if (msaaCounts & VK_SAMPLE_COUNT_32_BIT) maxMsaaSamples = VK_SAMPLE_COUNT_32_BIT;
    if (msaaCounts & VK_SAMPLE_COUNT_64_BIT) maxMsaaSamples = VK_SAMPLE_COUNT_64_BIT;

    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue computeQueue = VK_NULL_HANDLE;
    VkQueue presentationQueue = VK_NULL_HANDLE;
    VkQueue transferQueue = VK_NULL_HANDLE;
    {
        vkGetDeviceQueue(device, graphicsQueueIndex, 0, &graphicsQueue);
        vkGetDeviceQueue(device, computeQueueIndex, 0, &computeQueue);
        vkGetDeviceQueue(device, presentationQueueIndex, 0, &presentationQueue);
        if(transferQueueIndex != kInvalidQueueIndex)
        {
            vkGetDeviceQueue(device, transferQueueIndex, 0, &transferQueue);
        }
    }
}
