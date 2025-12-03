#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <cstdlib>

#include "SM/Engine.h"
#include "SM/Assert.h"
#include "SM/Platform.h"
#include "SM/Memory.h"
#include "SM/Bits.h"

#include "SM/Memory.cpp"

//-----------------------------------------------------------------------------------------------------
// Vulkan
#define VK_NO_PROTOTYPES
#include "ThirdParty/vulkan/vulkan.h"
#include "ThirdParty/vulkan/vulkan_win32.h"
#define VK_EXPORTED_FUNCTION(func)	PFN_##func func = VK_NULL_HANDLE;
#define VK_GLOBAL_FUNCTION(func)	PFN_##func func = VK_NULL_HANDLE;
#define VK_INSTANCE_FUNCTION(func)	PFN_##func func = VK_NULL_HANDLE;
#define VK_DEVICE_FUNCTION(func)	PFN_##func func = VK_NULL_HANDLE;
#include "SM/VulkanFunctionsManifest.inl"

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


using namespace SM;

static bool s_bExit = false;

void SM::Platform::Log(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	static const int MAX_BUFFER_SIZE = 2048;
	char formatted_msg[MAX_BUFFER_SIZE];
	SM_ASSERT(vsnprintf_s(formatted_msg, MAX_BUFFER_SIZE, format, args) != -1);
	va_end(args);

	OutputDebugStringA(formatted_msg);
}

bool SM::Platform::AssertReportFailure(const char* expression, const char* filename, I32 lineNumber)
{
	char assertMsg[MAX_ASSERT_MSG_LEN];
	sprintf_s(assertMsg, "Failure triggered at File: %s\nLine %i\n\nWould you like to debug? (Cancel quits program)", filename, lineNumber);

	int userBtnPressed = MessageBoxExA(NULL, assertMsg, "Assertion Failed", MB_YESNOCANCEL | MB_ICONERROR, 0);

	// kill program
	if (userBtnPressed == IDCANCEL)
	{
		exit(EXIT_FAILURE);
	}

	return (userBtnPressed == IDYES);
}

bool SM::Platform::AssertReportFailureMsg(const char* expression, const char* msg, const char* filename, I32 lineNumber)
{
	char assertMsg[MAX_ASSERT_MSG_LEN];
	sprintf_s(assertMsg, "%s\n\nFile: %s\nLine %i\nExpression \"%s\" failed.\n\nWould you like to debug? (Cancel quits program)", msg, filename, lineNumber, expression);

	int userBtnPressed = MessageBoxExA(NULL, assertMsg, "Error Triggered", MB_YESNOCANCEL | MB_ICONERROR, 0);

	// kill program
	if (userBtnPressed == IDCANCEL)
	{
		exit(EXIT_FAILURE);
	}

	return (userBtnPressed == IDYES);
}

bool SM::Platform::AssertReportError(const char* filename, I32 lineNumber)
{
	char assertMsg[MAX_ASSERT_MSG_LEN];
	sprintf_s(assertMsg, "Error triggered at File: %s\nLine %i\n\nWould you like to debug? (Cancel quits program)", filename, lineNumber);

	int userBtnPressed = MessageBoxExA(NULL, assertMsg, "Assertion Failed", MB_YESNOCANCEL | MB_ICONERROR, 0);

	// kill program
	if (userBtnPressed == IDCANCEL)
	{
		exit(EXIT_FAILURE);
	}

	return (userBtnPressed == IDYES);
}

bool SM::Platform::AssertReportErrorMsg(const char* msg, const char* filename, I32 lineNumber)
{
	char assertMsg[MAX_ASSERT_MSG_LEN];
	sprintf_s(assertMsg, "%s\n\nError triggered at File: %s\nLine %i\n\nWould you like to debug? (Cancel quits program)", msg, filename, lineNumber);

	int userBtnPressed = MessageBoxExA(NULL, assertMsg, "Error Triggered", MB_YESNOCANCEL | MB_ICONERROR, 0);

	// kill program
	if (userBtnPressed == IDCANCEL)
	{
		exit(EXIT_FAILURE);
	}

	return (userBtnPressed == IDYES);
}

void SM::Platform::TriggerDebugger()
{
	__debugbreak();
}

void SM::Platform::Exit()
{
    s_bExit = true;
}

static LRESULT EngineWinProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;

    switch (message)
    {
        case WM_SIZE:
        {
            result = DefWindowProc(window, message, wParam, lParam);
        }
        break;

        case WM_DESTROY:
        {
            result = DefWindowProc(window, message, wParam, lParam);
        }
        break;

        case WM_CLOSE:
        {
            SM::Platform::Exit();
        }
        break;

        case WM_ACTIVATEAPP:
        {
            result = DefWindowProc(window, message, wParam, lParam);
        }
        break;

        default:
        {
            result = DefWindowProc(window, message, wParam, lParam);
        }
        break;
    }

    return result;
}

struct GameApi
{
    GameBindEngineFunction GameBindEngine = nullptr;
    GameInitFunction GameInit = nullptr;
    GameUpdateFunction GameUpdate = nullptr;
    GameRenderFunction GameRender = nullptr;
    FILETIME m_dllLastWriteTime;
    bool m_bIsLoaded = false;
};

static void ReportLastWindowsError()
{
    DWORD errorCode = ::GetLastError();
    LPSTR errorString;
    DWORD numCharsWritten = ::FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, 
            NULL, 
            errorCode, 
            LANG_USER_DEFAULT, 
            (LPSTR)&errorString, 
            0, 
            NULL);
    
    if(numCharsWritten == 0)
    {
        DWORD formatError = ::GetLastError();
        Platform::Log("[Windows Error %i] Failed to generate error message for error code %i", formatError, errorCode);
    }
    else
    {
        Platform::Log("[Windows Error %i] %s", errorCode, errorString);
    }
}

static bool LoadGameCode(const char* dllFilename, GameApi& gameApi)
{
    if(!gameApi.m_bIsLoaded)
    {
        gameApi.GameBindEngine = [](SM::EngineApi engineApi){ };
        gameApi.GameInit   = [](){ };
        gameApi.GameUpdate = [](){ };
        gameApi.GameRender = [](){ };
    }

    WIN32_FILE_ATTRIBUTE_DATA fileInfo = {};
    bool gotFileInfo = ::GetFileAttributesExA(dllFilename, GetFileExInfoStandard, &fileInfo);
    if(!gotFileInfo)
    {
        Platform::Log("[Error] Failed to get file attributes for dll %s", dllFilename);
        ReportLastWindowsError();
        return false;
    }

    if(::CompareFileTime(&fileInfo.ftLastWriteTime, &gameApi.m_dllLastWriteTime) == 0 && gameApi.m_bIsLoaded)
    {
        return true;
    }

    const char* tempFilename = "Working.dll";
    BOOL copiedToWorking = ::CopyFileA(dllFilename, tempFilename, FALSE);
    if(!copiedToWorking)
    {
        return false;
    }

    HMODULE gameDLL = ::LoadLibraryA(tempFilename);
    SM_ASSERT(gameDLL != NULL);
    if(gameDLL == NULL)
    {
        Platform::Log("Not a valid dll\n");
        return false;
    }

    GameBindEngineFunction GameBindEngine = (GameBindEngineFunction)::GetProcAddress(gameDLL, GAME_BIND_ENGINE_FUNCTION_NAME_STRING);
    if(GameBindEngine == NULL)
    {
        Platform::Log("Couldn't find %s in DLL\n", GAME_BIND_ENGINE_FUNCTION_NAME_STRING);
        return false;
    }

    GameInitFunction GameInit = (GameInitFunction)::GetProcAddress(gameDLL, GAME_INIT_FUNCTION_NAME_STRING);
    if(GameInit == NULL)
    {
        Platform::Log("Couldn't find %s in DLL\n", GAME_INIT_FUNCTION_NAME_STRING);
        return false;
    }

    GameUpdateFunction GameUpdate = (GameUpdateFunction)::GetProcAddress(gameDLL, GAME_UPDATE_FUNCTION_NAME_STRING);
    if(GameUpdate == NULL)
    {
        Platform::Log("Couldn't find %s in DLL\n", GAME_UPDATE_FUNCTION_NAME_STRING);
        return false;
    }

    GameRenderFunction GameRender = (GameRenderFunction)::GetProcAddress(gameDLL, GAME_RENDER_FUNCTION_NAME_STRING);
    if(GameRender == NULL)
    {
        Platform::Log("Couldn't find %s in DLL\n", GAME_RENDER_FUNCTION_NAME_STRING);
        return false;
    }

    gameApi.GameBindEngine = GameBindEngine;
    gameApi.GameInit = GameInit;
    gameApi.GameUpdate = GameUpdate;
    gameApi.GameRender = GameRender;
    gameApi.m_dllLastWriteTime = fileInfo.ftLastWriteTime;

    EngineApi engineApi = {
        .EngineLog = &Platform::Log
    };
    gameApi.GameBindEngine(engineApi);

    if(!gameApi.m_bIsLoaded)
    {
        gameApi.m_bIsLoaded = true;
        gameApi.GameInit();
    }

    return true;
}

struct Win32Window
{
    HWND m_hwnd;
    const char* m_title;
    U32 m_width;
    U32 m_height;
};

static Win32Window OpenWindow(const char* windowTitle, U32 width, U32 height)
{
	::SetProcessDPIAware(); // make sure window is created with scaling handled

    static const char* windowClassName = "SM Engine Window Class";

	WNDCLASSEX wc = {
        .cbSize = sizeof(wc),
        .style = CS_OWNDC | CS_DBLCLKS,
        .lpfnWndProc = EngineWinProc,
        .hInstance = ::GetModuleHandle(NULL),
        .hCursor = ::LoadCursor(NULL, IDC_ARROW),
        .lpszClassName = windowClassName 
    };
	RegisterClassEx(&wc);

	DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_VISIBLE;
    bool bResizable = true;
	if (bResizable)
	{
		style |= WS_THICKFRAME;
	}

	// calculate how big to create the window to ensure the "client area" (renderable surface) is actually sized to width & height params
	RECT fullSize;
	fullSize.left = 0;
	fullSize.top = 0;
	fullSize.right = width;
	fullSize.bottom = height;

	::AdjustWindowRectEx(&fullSize, style, FALSE, NULL);
	I32 createWidth = fullSize.right - fullSize.left;
	I32 createHeight = fullSize.bottom - fullSize.top;

	Win32Window window = {};
	window.m_title = windowTitle;
    window.m_width = width;
    window.m_height = height;
	window.m_hwnd = ::CreateWindowExA(0,
							          windowClassName,
							          windowTitle,
							          style,
							          CW_USEDEFAULT,
							          CW_USEDEFAULT,
							          createWidth,
							          createHeight,
							          NULL, 
                                      NULL,
							          GetModuleHandle(NULL),
							          NULL); // used to pass this pointer
    if(window.m_hwnd == NULL)
    {
        ReportLastWindowsError();
        SM_ERROR_MSG("Failed to create window\n");
    }

	// make sure to show on init
	::ShowWindow(window.m_hwnd, SW_SHOW);
	::BringWindowToTop(window.m_hwnd);

    return window;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT      msgSeverity,
													      VkDebugUtilsMessageTypeFlagsEXT             msgType,
													      const VkDebugUtilsMessengerCallbackDataEXT* cbData,
													      void*										  userData)
{
	UNUSED(msgType);
	UNUSED(userData);

	// filter out verbose and info messages unless we explicitly want them
	if (!VULKAN_VERBOSE)
	{
		if (msgSeverity & (VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT))
		{
			return VK_FALSE;
		}
	}

    Platform::Log("[vk");

	switch (msgType)
	{
		case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:		Platform::Log("-general");		break;
		case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:	Platform::Log("-performance");	break;
		case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:	Platform::Log("-validation");	break;
	}

	switch (msgSeverity)
	{
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:	Platform::Log("-verbose]");	break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:		Platform::Log("-info]");	break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:	Platform::Log("-warning]");	break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:		Platform::Log("-error]");	break;
		//case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT:
		default: break;
	}

    Platform::Log(" %s\n", cbData->pMessage);

	// returning false means we don't abort the Vulkan call that triggered the debug callback
	return VK_FALSE;
}

int WINAPI WinMain(HINSTANCE app, 
				   HINSTANCE prevApp, 
				   LPSTR args, 
				   int show)
{
    using namespace SM;
    const char* dllName = "Workbench.dll";

    /*
       For future reference of how to ask platform for memory info
        SYSTEM_INFO systemInfo;
        ::GetSystemInfo(&systemInfo);
        U32 numProcessors = systemInfo.dwNumberOfProcessors;
        U32 pageSize = systemInfo.dwPageSize;
        U32 allocationGranularity = systemInfo.dwAllocationGranularity;
    */

    SM::InitAllocators();
    Win32Window window = OpenWindow("Workbench", 1600, 900);

    // initialize Vulkan
    {
        #define VK_EXPORTED_FUNCTION(func) \
            HMODULE vulkan_lib = ::LoadLibraryA("vulkan-1.dll"); \
            SM_ASSERT(nullptr != vulkan_lib); \
            func = (PFN_##func)::GetProcAddress(vulkan_lib, #func); \
            SM_ASSERT(nullptr != func);

        #define VK_GLOBAL_FUNCTION(func) \
            func = (PFN_##func)vkGetInstanceProcAddr(nullptr, #func); \
            SM_ASSERT(nullptr != func);

        #include "SM/VulkanFunctionsManifest.inl"

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

        static const U8 maxNumGPUs = 5; // bro do you really have more than 5 graphics cards

        U32 numFoundGPUs = 0;
        vkEnumeratePhysicalDevices(instance, &numFoundGPUs, nullptr);
        SM_ASSERT(numFoundGPUs != 0 && numFoundGPUs <= maxNumGPUs);

        VkPhysicalDevice foundGPUs[maxNumGPUs];
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

        for(const VkPhysicalDevice& candidateGPU : foundGPUs)
        {
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

            //render_queue_indices_t queue_indices = find_queue_indices(arena, device, surface);
            //return has_required_queues(queue_indices);

            U32 numQueueFamilies = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(candidateGPU, &numQueueFamilies, nullptr);

            VkQueueFamilyProperties* queueFamilyProperties = SM::Alloc<VkQueueFamilyProperties>(kEngineGlobal, numQueueFamilies);
            vkGetPhysicalDeviceQueueFamilyProperties(candidateGPU, &numQueueFamilies, queueFamilyProperties);

            static const I32 kInvalidQueue = -1;
            I32 graphicsQueue = kInvalidQueue;
            I32 computeQueue = kInvalidQueue;
            I32 transferQueue = kInvalidQueue;
            I32 presentationQueue = kInvalidQueue;
            for (int iQueue = 0; iQueue < numQueueFamilies; iQueue++)
            {
                const VkQueueFamilyProperties& props = queueFamilyProperties[iQueue];

                if (graphicsQueue == kInvalidQueue && IsBitSet(props.queueFlags, VK_QUEUE_GRAPHICS_BIT))
                {
                    graphicsQueue = iQueue;
                }

                if (computeQueue == kInvalidQueue && 
                    IsBitSet(props.queueFlags, VK_QUEUE_COMPUTE_BIT)  && 
                    !IsBitSet(props.queueFlags, VK_QUEUE_GRAPHICS_BIT))
                {
                    computeQueue = iQueue;
                }

                if (transferQueue == kInvalidQueue && 
                    IsBitSet(props.queueFlags, VK_QUEUE_TRANSFER_BIT) &&
                    !IsBitSet(props.queueFlags, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT))
                {
                    transferQueue = iQueue;
                }

                VkBool32 canPresent = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(candidateGPU, iQueue, surface, &canPresent);
                if (presentationQueue == kInvalidQueue && canPresent)
                {
                    presentationQueue = iQueue;
                }

                // check if no more families to find
                if (graphicsQueue != kInvalidQueue && 
                    computeQueue != kInvalidQueue &&
                    presentationQueue != kInvalidQueue &&
                    transferQueue != kInvalidQueue)
                {
                    break;
                }
            }

            if(graphicsQueue == -1 || presentationQueue == -1)
            {
                continue;
            }

            //if(is_physical_device_suitable(arena, device, context.surface))
            //{
            //    selectedGPU = device;
            //    vkGetPhysicalDeviceProperties(selectedGPU, &gpuProperties);
            //    vkGetPhysicalDeviceMemoryProperties(selectedGPU, &gpuMemoryProperties);
            //    queue_indices = find_queue_indices(arena, device, context.surface);
            //    max_num_msaa_samples = get_max_msaa_samples(selected_phys_device_props);
            //    break;
            //}
        }

        //SM_ASSERT(selectedGPU != VK_NULL_HANDLE);

        VkPhysicalDeviceFeatures deviceFeatures = {
            .fillModeNonSolid = VK_TRUE,
            .samplerAnisotropy = VK_TRUE
        };

        VkPhysicalDeviceVulkan13Features vk13Features = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
            .synchronization2 = VK_TRUE,
            .dynamicRendering = VK_TRUE
        };

        VkDeviceCreateInfo createInfo = {
            
        };
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pNext = &vk13Features;
        //createInfo.pQueueCreateInfos = queue_create_infos;
        //createInfo.queueCreateInfoCount = num_queues;
        createInfo.pEnabledFeatures = &deviceFeatures;
        createInfo.enabledExtensionCount = ARRAY_LEN(DEVICE_EXTENSIONS);
        createInfo.ppEnabledExtensionNames = DEVICE_EXTENSIONS;

        VkDevice device = VK_NULL_HANDLE;
        //SM_ASSERT(vkCreateDevice(selectedGPU, &createInfo, nullptr, &device) == VK_SUCCESS);

        // load device funcs
        //#define VK_DEVICE_FUNCTION(func) \
        //    func = (PFN_##func)vkGetDeviceProcAddr(device, #func); \
        //    SM_ASSERT(nullptr != func);

        //#include "SM/VulkanFunctionsManifest.inl"
    }

    GameApi gameApi = {};
    while(!s_bExit)
    {
        MSG msg;
        while (::PeekMessage(&msg, window.m_hwnd, 0, 0, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }

        LoadGameCode(dllName, gameApi);
        gameApi.GameUpdate();
        gameApi.GameRender();
    }

    return 0;
}
