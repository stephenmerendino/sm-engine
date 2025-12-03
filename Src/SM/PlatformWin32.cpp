#include "SM/Platform.h"
#include "SM/Assert.h"
#include "SM/Engine.h"
#include "SM/Memory.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <cstdlib>

#define VK_PLATFORM_FUNCTIONS

#include "ThirdParty/vulkan/vulkan_win32.h"
#define VK_EXPORTED_FUNCTION(func)	PFN_##func func = VK_NULL_HANDLE;
#define VK_GLOBAL_FUNCTION(func)	PFN_##func func = VK_NULL_HANDLE;
#define VK_INSTANCE_FUNCTION(func)	PFN_##func func = VK_NULL_HANDLE;
#define VK_DEVICE_FUNCTION(func)	PFN_##func func = VK_NULL_HANDLE;
#include "SM/Renderer/VulkanFunctionsManifest.inl"

#include "SM/Renderer/VulkanConfig.h"

using namespace SM;

/*
   For future reference of how to ask platform for memory info
    SYSTEM_INFO systemInfo;
    ::GetSystemInfo(&systemInfo);
    U32 numProcessors = systemInfo.dwNumberOfProcessors;
    U32 pageSize = systemInfo.dwPageSize;
    U32 allocationGranularity = systemInfo.dwAllocationGranularity;
*/

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

struct Platform::Window
{
    HWND m_hwnd;
    const char* m_title;
    U32 m_width;
    U32 m_height;
};

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
            SM::EngineExit();
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

Platform::Window* Platform::OpenWindow(const char* title, U32 width, U32 height)
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

	Window* pWindow = SM::Alloc<Window>(kEngineGlobal);
	pWindow->m_title =title;
    pWindow->m_width = width;
    pWindow->m_height = height;
	pWindow->m_hwnd = ::CreateWindowExA(0,
							          windowClassName,
							          title,
							          style,
							          CW_USEDEFAULT,
							          CW_USEDEFAULT,
							          createWidth,
							          createHeight,
							          NULL, 
                                      NULL,
							          GetModuleHandle(NULL),
							          NULL); // used to pass this pointer
    if(pWindow->m_hwnd == NULL)
    {
        ReportLastWindowsError();
        SM_ERROR_MSG("Failed to create window\n");
    }

	// make sure to show on init
	::ShowWindow(pWindow->m_hwnd, SW_SHOW);
	::BringWindowToTop(pWindow->m_hwnd);

    return pWindow;
}

GameApi Platform::LoadGameDll(const char* gameDll)
{
    static bool bIsLoaded = false;
    static FILETIME lastLoadFiletime;
    static GameApi gameApi = {
        .GameBindEngine = [](SM::EngineApi engineApi){ },
        .GameInit   = [](){ },
        .GameUpdate = [](){ },
        .GameRender = [](){ }
    };

    WIN32_FILE_ATTRIBUTE_DATA fileInfo = {};
    bool gotFileInfo = ::GetFileAttributesExA(gameDll, GetFileExInfoStandard, &fileInfo);
    if(!gotFileInfo)
    {
        Platform::Log("[Game DLL] Failed to get file attributes for dll %s", gameDll);
        ReportLastWindowsError();
        return gameApi;
    }

    if(::CompareFileTime(&fileInfo.ftLastWriteTime, &lastLoadFiletime) == 0 && bIsLoaded)
    {
        return gameApi;
    }

    const char* tempFilename = "Working.dll";
    BOOL copiedToWorking = ::CopyFileA(gameDll, tempFilename, FALSE);
    if(!copiedToWorking)
    {
        Platform::Log("[Game DLL] Failed to copy dll %s to working dll", gameDll);
        return gameApi;
    }

    HMODULE gameDLL = ::LoadLibraryA(tempFilename);
    SM_ASSERT(gameDLL != NULL);
    if(gameDLL == NULL)
    {
        Platform::Log("[Game DLL] Failed to load dll %s", tempFilename);
        return gameApi;
    }

    GameBindEngineFunction GameBindEngine = (GameBindEngineFunction)::GetProcAddress(gameDLL, GAME_BIND_ENGINE_FUNCTION_NAME_STRING);
    if(GameBindEngine == NULL)
    {
        Platform::Log("[Game DLL] Couldn't find function %s\n", GAME_BIND_ENGINE_FUNCTION_NAME_STRING);
        return gameApi;
    }

    GameInitFunction GameInit = (GameInitFunction)::GetProcAddress(gameDLL, GAME_INIT_FUNCTION_NAME_STRING);
    if(GameInit == NULL)
    {
        Platform::Log("[Game DLL] Couldn't find function %s\n", GAME_INIT_FUNCTION_NAME_STRING);
        return gameApi;
    }

    GameUpdateFunction GameUpdate = (GameUpdateFunction)::GetProcAddress(gameDLL, GAME_UPDATE_FUNCTION_NAME_STRING);
    if(GameUpdate == NULL)
    {
        Platform::Log("[Game DLL] Couldn't find function %s\n", GAME_UPDATE_FUNCTION_NAME_STRING);
        return gameApi;
    }

    GameRenderFunction GameRender = (GameRenderFunction)::GetProcAddress(gameDLL, GAME_RENDER_FUNCTION_NAME_STRING);
    if(GameRender == NULL)
    {
        Platform::Log("[Game DLL] Couldn't find function %s\n", GAME_RENDER_FUNCTION_NAME_STRING);
        return gameApi;
    }

    gameApi.GameBindEngine = GameBindEngine;
    gameApi.GameInit = GameInit;
    gameApi.GameUpdate = GameUpdate;
    gameApi.GameRender = GameRender;
    lastLoadFiletime = fileInfo.ftLastWriteTime;

    EngineApi engineApi = {
        .Log = &Platform::Log
    };
    gameApi.GameBindEngine(engineApi);

    return gameApi;
}

void Platform::UpdateWindow(Platform::Window* pWindow)
{
    MSG msg;
    while (::PeekMessage(&msg, pWindow->m_hwnd, 0, 0, PM_REMOVE))
    {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
    }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL Win32VulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT msgSeverity,
													           VkDebugUtilsMessageTypeFlagsEXT msgType,
													           const VkDebugUtilsMessengerCallbackDataEXT* cbData,
													           void* userData)
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

PFN_vkDebugUtilsMessengerCallbackEXT Platform::GetVulkanDebugCallback()
{
    return Win32VulkanDebugCallback;
}

void Platform::LoadVulkanGlobalFuncs()
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
}

void Platform::LoadVulkanInstanceFuncs(VkInstance instance)
{
    // load instance funcs
    #define VK_INSTANCE_FUNCTION(func) \
        func = (PFN_##func)vkGetInstanceProcAddr(instance, #func); \
        SM_ASSERT(nullptr != func);
    #include "SM/Renderer/VulkanFunctionsManifest.inl"
}

void Platform::LoadVulkanDeviceFuncs(VkDevice device)
{
    #define VK_DEVICE_FUNCTION(func) \
        func = (PFN_##func)vkGetDeviceProcAddr(device, #func); \
        SM_ASSERT(nullptr != func);

    #include "SM/Renderer/VulkanFunctionsManifest.inl"
}

VkSurfaceKHR Platform::CreateVulkanSurface(VkInstance instance, Window* platformWindow)
{
    // vk surface
    VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .pNext = nullptr,
        .hinstance = ::GetModuleHandle(nullptr),
        .hwnd = platformWindow->m_hwnd
    };
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    SM_ASSERT(vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo, nullptr, &surface) == VK_SUCCESS);
    return surface;
}
