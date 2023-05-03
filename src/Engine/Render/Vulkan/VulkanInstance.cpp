#include "Engine/Render/Vulkan/VulkanInstance.h"
#include "Engine/Render/Vulkan/VulkanUtils.h"
#include "Engine/Core/Assert.h"
#include "Engine/Core/Common.h"
#include "Engine/Core/Config.h"

VkInstance VulkanInstance::m_vkInstance = VK_NULL_HANDLE;
VkDebugUtilsMessengerEXT VulkanInstance::m_debugMessenger = VK_NULL_HANDLE;

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

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCb(VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
											 VkDebugUtilsMessageTypeFlagsEXT             messageType,
											 const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
											 void*                                       pUserData)
{
	Unused(messageType);
	Unused(pUserData);

	// filter out verbose and info messages unless we explicitly want them
	if (!VULKAN_VERBOSE)
	{
		if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT ||
			messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
		{
			return VK_FALSE;
		}
	}

	DebugPrintf("[Vk");

	switch (messageType)
	{
		case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:		DebugPrintf("-General");		break;
		case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:	DebugPrintf("-Performance");	break;
		case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:	DebugPrintf("-Validation");	break;
	}

	switch (messageSeverity)
	{
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:	DebugPrintf("-Verbose]");	break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:		DebugPrintf("-Info]");		break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:	DebugPrintf("-Warning]");	break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:		DebugPrintf("-Error]");		break;
	}

	{
		DebugPrintf(" %s\n", pCallbackData->pMessage);
	}

	// returning false means we don't abort the Vulkan call that triggered the debug callback
	return VK_FALSE;
}

bool VulkanInstance::Setup()
{
	LoadVulkanGlobalFuncs();

	PrintInstanceInfo();

	// app info
	VkApplicationInfo appInfo;
	MemZero(appInfo);
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Workbench";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_3;

	// create info
	VkInstanceCreateInfo createInfo;
	MemZero(createInfo);
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	// extensions
	createInfo.ppEnabledExtensionNames = INSTANCE_EXTENSIONS.data();
	createInfo.enabledExtensionCount = (U32)INSTANCE_EXTENSIONS.size();

	// validation layers
	if (ENABLE_VALIDATION_LAYERS)
	{
		ASSERT(CheckValidationLayerSupport());
		createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
		createInfo.enabledLayerCount = (U32)VALIDATION_LAYERS.size();

		VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo = SetupDebugMessengerCreateInfo(VulkanDebugCb);
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

	VULKAN_ASSERT(vkCreateInstance(&createInfo, nullptr, &m_vkInstance));

	LoadVulkanInstanceFuncs(m_vkInstance);

	// debug messenger
	if (ENABLE_VALIDATION_LAYERS)
	{
		VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo = SetupDebugMessengerCreateInfo(VulkanDebugCb);
		VULKAN_ASSERT(vkCreateDebugUtilsMessengerEXT(m_vkInstance, &debugMessengerCreateInfo, nullptr, &m_debugMessenger));
	}

	return true;
}

void VulkanInstance::Teardown()
{
	if (m_debugMessenger != VK_NULL_HANDLE)
	{
		vkDestroyDebugUtilsMessengerEXT(m_vkInstance, m_debugMessenger, nullptr);
	}
	vkDestroyInstance(m_vkInstance, nullptr);
}

VkInstance VulkanInstance::GetHandle()
{
	ASSERT(m_vkInstance != VK_NULL_HANDLE);
	return m_vkInstance;
}