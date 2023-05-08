//#include "engine/core/Common.h"
//#include "engine/core/Config.h"
//#include "engine/core/Debug.h"
//#include "engine/render/vulkan/VulkanRenderer2.h"
//#include "engine/render/vulkan/VulkanInclude.h"
//
//namespace sm
//{
//    struct vulkan_instance_t
//    {
//        VkInstance m_instance = VK_NULL_HANDLE;
//        VkDebugUtilsMessengerEXT m_debug_messenger = VK_NULL_HANDLE;
//    };
//
//    static void print_instance_info()
//    {
//        if (!is_debug())
//        {
//            return;
//        }
//
//        // print supported extensions
//        {
//            U32 numExtensions;
//            vkEnumerateInstanceExtensionProperties(nullptr, &numExtensions, nullptr);
//
//            std::vector<VkExtensionProperties> instanceExtensions(numExtensions);
//            vkEnumerateInstanceExtensionProperties(nullptr, &numExtensions, instanceExtensions.data());
//
//            DebugPrintf("Supported Instance Extensions\n");
//            for (const VkExtensionProperties& p : instanceExtensions)
//            {
//                DebugPrintf("  Name: %s | Spec Version: %i\n", p.extensionName, p.specVersion);
//            }
//        }
//
//        // print supported validation layers
//        {
//            U32 numLayers;
//            vkEnumerateInstanceLayerProperties(&numLayers, nullptr);
//
//            std::vector<VkLayerProperties> instanceLayers(numLayers);
//            vkEnumerateInstanceLayerProperties(&numLayers, instanceLayers.data());
//
//            DebugPrintf("Supported Instance Validation Layers\n");
//            for (const VkLayerProperties& l : instanceLayers)
//            {
//                DebugPrintf("  Name: %s | Desc: %s | Spec Version: %i\n", l.layerName, l.description, l.specVersion);
//            }
//        }
//    }
//
//    static bool CheckValidationLayerSupport()
//    {
//        U32 numLayers;
//        vkEnumerateInstanceLayerProperties(&numLayers, nullptr);
//
//        std::vector<VkLayerProperties> instanceLayers(numLayers);
//        vkEnumerateInstanceLayerProperties(&numLayers, instanceLayers.data());
//
//        for (const char* layerName : VALIDATION_LAYERS)
//        {
//            bool layerFound = false;
//
//            for (const VkLayerProperties& l : instanceLayers)
//            {
//                if (strcmp(layerName, l.layerName) == 0)
//                {
//                    layerFound = true;
//                    break;
//                }
//            }
//
//            if (!layerFound)
//            {
//                return false;
//            }
//        }
//
//        return true;
//    }
//
//    VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCb(VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
//                                                 VkDebugUtilsMessageTypeFlagsEXT             messageType,
//                                                 const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
//                                                 void*                                       pUserData)
//    {
//        Unused(messageType);
//        Unused(pUserData);
//
//        // filter out verbose and info messages unless we explicitly want them
//        if (!VULKAN_VERBOSE)
//        {
//            if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT ||
//                messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
//            {
//                return VK_FALSE;
//            }
//        }
//
//        DebugPrintf("[Vk");
//
//        switch (messageType)
//        {
//            case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:		DebugPrintf("-General");		break;
//            case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:	DebugPrintf("-Performance");	break;
//            case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:	DebugPrintf("-Validation");	break;
//        }
//
//        switch (messageSeverity)
//        {
//            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:	DebugPrintf("-Verbose]");	break;
//            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:		DebugPrintf("-Info]");		break;
//            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:	DebugPrintf("-Warning]");	break;
//            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:		DebugPrintf("-Error]");		break;
//        }
//
//        {
//            DebugPrintf(" %s\n", pCallbackData->pMessage);
//        }
//
//        // returning false means we don't abort the Vulkan call that triggered the debug callback
//        return VK_FALSE;
//    }
//
//    void SetupRenderer()
//    {
//        
//    }
//
//    void RenderFrame()
//    {
//        
//    }
//}
