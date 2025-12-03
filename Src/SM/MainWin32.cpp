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
#include "SM/Renderer/Renderer.cpp"


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
