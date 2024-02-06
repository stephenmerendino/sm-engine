#include "Engine/Render/Vulkan/VulkanInstance.h"
#include "Engine/Config.h"
#include "Engine/Core/Assert.h"
#include "Engine/Core/Debug.h"
#include "Engine/Core/Macros.h"
#include "Engine/Core/Types.h"

VulkanInstance* VulkanInstance::s_instance = nullptr;

static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugFunc(VkDebugUtilsMessageSeverityFlagBitsEXT      msgSeverity,
													  VkDebugUtilsMessageTypeFlagsEXT             msgType,
													  const VkDebugUtilsMessengerCallbackDataEXT* pCbData,
													  void*										  pUserData)
{
	UNUSED(msgType);
	UNUSED(pUserData);

	// filter out verbose and info messages unless we explicitly want them
	if (!VULKAN_VERBOSE)
	{
		if (msgSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT ||
			msgSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
		{
			return VK_FALSE;
		}
	}

	DebugPrintf("[Vk");

	switch (msgType)
	{
		case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:		DebugPrintf("-General");		break;
		case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:	DebugPrintf("-Performance");	break;
		case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:	DebugPrintf("-Validation");		break;
	}

	switch (msgSeverity)
	{
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:	DebugPrintf("-Verbose]");	break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:		DebugPrintf("-Info]");		break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:	DebugPrintf("-Warning]");	break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:		DebugPrintf("-Error]");		break;
		//case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT:
		default: break;
	}

	DebugPrintf(" %s\n", pCbData->pMessage);

	// returning false means we don't abort the Vulkan call that triggered the debug callback
	return VK_FALSE;
}

static VkDebugUtilsMessengerCreateInfoEXT SetupDebugMessengerCreateInfo(PFN_vkDebugUtilsMessengerCallbackEXT callback)
{
	VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

	createInfo.pfnUserCallback = callback;
	createInfo.pUserData = nullptr;

	return 	createInfo;
}

static bool CheckValidationLayerSupport()
{
	U32 numLayers;
	vkEnumerateInstanceLayerProperties(&numLayers, nullptr);

	std::vector<VkLayerProperties> instanceLayers(numLayers);
	vkEnumerateInstanceLayerProperties(&numLayers, instanceLayers.data());

	for (const char* layerName : VALIDATION_LAYERS)
	{
		bool layerFound = false;

		for (const VkLayerProperties& l : instanceLayers)
		{
			if (strcmp(layerName, l.layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}

		if (!layerFound)
		{
			return false;
		}
	}

	return true;
}

static void PrintInstanceInfo()
{
	if (!IsDebug())
	{
		return;
	}

	// print supported extensions
	{
		U32 numExtensions;
		vkEnumerateInstanceExtensionProperties(nullptr, &numExtensions, nullptr);

		std::vector<VkExtensionProperties> instanceExtensions(numExtensions);
		vkEnumerateInstanceExtensionProperties(nullptr, &numExtensions, instanceExtensions.data());

		DebugPrintf("Supported Instance Extensions\n");
		for (const VkExtensionProperties& p : instanceExtensions)
		{
			DebugPrintf("  Name: %s | Spec Version: %i\n", p.extensionName, p.specVersion);
		}
	}

	// print supported validation layers
	{
		U32 numLayers;
		vkEnumerateInstanceLayerProperties(&numLayers, nullptr);

		std::vector<VkLayerProperties> instanceLayers(numLayers);
		vkEnumerateInstanceLayerProperties(&numLayers, instanceLayers.data());

		DebugPrintf("Supported Instance Validation Layers\n");
		for (const VkLayerProperties& l : instanceLayers)
		{
			DebugPrintf("  Name: %s | Desc: %s | Spec Version: %i\n", l.layerName, l.description, l.specVersion);
		}
	}
}

VulkanInstance::VulkanInstance()
	:m_instanceHandle(VK_NULL_HANDLE)
	,m_debugMessengerHandle(VK_NULL_HANDLE)
{
}

void VulkanInstance::Init()
{
	LoadVulkanGlobalFuncs();

	PrintInstanceInfo();

	// app info
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Workbench";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_3;

	// create info
	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	// extensions
	createInfo.ppEnabledExtensionNames = INSTANCE_EXTENSIONS.data();
	createInfo.enabledExtensionCount = (U32)INSTANCE_EXTENSIONS.size();

	// validation layers
	if (ENABLE_VALIDATION_LAYERS)
	{
		SM_ASSERT(CheckValidationLayerSupport());
		createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
		createInfo.enabledLayerCount = (U32)VALIDATION_LAYERS.size();

		// this debug messenger debugs the actual instance creation
		VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo = SetupDebugMessengerCreateInfo(VulkanDebugFunc);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugMessengerCreateInfo;

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

	s_instance = new VulkanInstance();
	SM_VULKAN_ASSERT(vkCreateInstance(&createInfo, nullptr, &s_instance->m_instanceHandle));

	LoadVulkanInstanceFuncs(s_instance->m_instanceHandle);

	// real debug messenger for the whole game
	if (ENABLE_VALIDATION_LAYERS)
	{
		VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo = SetupDebugMessengerCreateInfo(VulkanDebugFunc);
		SM_VULKAN_ASSERT(vkCreateDebugUtilsMessengerEXT(s_instance->m_instanceHandle, &debugMessengerCreateInfo, nullptr, &s_instance->m_debugMessengerHandle));
	}
}

void VulkanInstance::Destroy()
{
	if (s_instance->m_debugMessengerHandle != VK_NULL_HANDLE)
	{
		vkDestroyDebugUtilsMessengerEXT(s_instance->m_instanceHandle, s_instance->m_debugMessengerHandle, nullptr);
	}
	vkDestroyInstance(s_instance->m_instanceHandle, nullptr);
	delete s_instance;
}

VulkanInstance* VulkanInstance::Get()
{
	return s_instance;
}

VkInstance VulkanInstance::GetHandle()
{
	return s_instance->m_instanceHandle;
}