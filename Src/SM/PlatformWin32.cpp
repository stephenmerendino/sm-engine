#include "SM/Platform.h"
#include "SM/Bits.h"
#include "SM/Assert.h"
#include "SM/Engine.h"
#include "SM/Memory.h"
#include "SM/Math.h"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#pragma warning(disable : 5039) // Disable weird 'TpSetCallbackCleanupGroup' windows error
#include <windows.h>

#define VK_PLATFORM_FUNCTIONS
#include "ThirdParty/vulkan/vulkan_win32.h"
#define VK_EXPORTED_FUNCTION(func)	PFN_##func func = VK_NULL_HANDLE;
#define VK_GLOBAL_FUNCTION(func)	PFN_##func func = VK_NULL_HANDLE;
#define VK_INSTANCE_FUNCTION(func)	PFN_##func func = VK_NULL_HANDLE;
#define VK_DEVICE_FUNCTION(func)	PFN_##func func = VK_NULL_HANDLE;
#include "SM/Renderer/VulkanFunctionsManifest.inl"
#include "SM/Renderer/VulkanConfig.h"
#include "ThirdParty/imgui/imgui_impl_win32.cpp"

#include <combaseapi.h>
#include "ThirdParty/dxc/dxcapi.h"
#include <atlbase.h>
#include <cstdio>
#include <cstdlib>

using namespace SM;

// timing
static I64 s_timingFreqPerSec = 0;

// shader compiler
CComPtr<IDxcLibrary> s_dxcShaderCompilerLibrary;
CComPtr<IDxcCompiler3> s_dxcShaderCompiler;
CComPtr<IDxcUtils> s_dxcUtils;

// input state
U8 s_keyStates[(U8)Platform::KeyCode::kNumKeyCodes] = { 0 };
Vec2 s_mouseMovementNormalized = Vec2::kZero;
IVec2 s_savedMousePos = IVec2::kZero;

struct MouseState
{
    IVec2 m_mousePosScreen;
    Vec2 m_mousePosScreenNormalized;
    IVec2 m_mousePosWindow;
    Vec2 m_mousePosWindowNormalized;
};

/*
   For future reference of how to ask platform for memory info
    SYSTEM_INFO systemInfo;
    ::GetSystemInfo(&systemInfo);
    U32 numProcessors = systemInfo.dwNumberOfProcessors;
    U32 pageSize = systemInfo.dwPageSize;
    U32 allocationGranularity = systemInfo.dwAllocationGranularity;
*/

static I32 Win32KeyToEngineKey(U32 windowsKey)
{
	// https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes

	// Numbers 0-9
	// 0x30 = 0, 0x39 = 9
	if (windowsKey >= 0x30 && windowsKey <= 0x39)
	{
		return (U32)Platform::KeyCode::kKey0 + (windowsKey - 0x30);
	}

	// Letters A-Z
	// 0x41 = A, 0x5A = Z
	if (windowsKey >= 0x41 && windowsKey <= 0x5A)
	{
		return (U32)Platform::KeyCode::kKeyA + (windowsKey - 0x41);
	}

	// Numpad 0-9
	if (windowsKey >= VK_NUMPAD0 && windowsKey <= VK_NUMPAD9)
	{
		return (U32)Platform::KeyCode::kKeyNumPad0 + (windowsKey - VK_NUMPAD0);
	}

	// F1-F24
	if (windowsKey >= VK_F1 && windowsKey <= VK_F24)
	{
		return (U32)Platform::KeyCode::kKeyF1 + (windowsKey - VK_F1);
	}

	// Handle everything else directly
	switch (windowsKey)
	{
        case VK_LBUTTON:    return (U32)Platform::KeyCode::kMouseLButton; 
		case VK_RBUTTON:    return (U32)Platform::KeyCode::kMouseRButton; 
		case VK_MBUTTON:    return (U32)Platform::KeyCode::kMouseMButton; 

		case VK_BACK:       return (U32)Platform::KeyCode::kKeyBackspace; 
		case VK_TAB:        return (U32)Platform::KeyCode::kKeyTab; 
		case VK_CLEAR:      return (U32)Platform::KeyCode::kKeyClear; 
		case VK_RETURN:     return (U32)Platform::KeyCode::kKeyEnter; 
		case VK_SHIFT:      return (U32)Platform::KeyCode::kKeyShift; 
		case VK_CONTROL:    return (U32)Platform::KeyCode::kKeyControl; 
		case VK_MENU:       return (U32)Platform::KeyCode::kKeyAlt; 
		case VK_PAUSE:      return (U32)Platform::KeyCode::kKeyPause; 
		case VK_CAPITAL:    return (U32)Platform::KeyCode::kKeyCapslock; 
		case VK_ESCAPE:     return (U32)Platform::KeyCode::kKeyEscape; 
		case VK_SPACE:      return (U32)Platform::KeyCode::kKeySpace; 
		case VK_PRIOR:      return (U32)Platform::KeyCode::kKeyPageUp; 
		case VK_NEXT:       return (U32)Platform::KeyCode::kKeyPageDown; 
		case VK_END:        return (U32)Platform::KeyCode::kKeyEnd; 
		case VK_HOME:       return (U32)Platform::KeyCode::kKeyHome; 
		case VK_LEFT:       return (U32)Platform::KeyCode::kKeyLeftArrow; 
		case VK_UP:         return (U32)Platform::KeyCode::kKeyUpArrow; 
		case VK_RIGHT:      return (U32)Platform::KeyCode::kKeyRightArrow; 
		case VK_DOWN:       return (U32)Platform::KeyCode::kKeyDownArrow; 
		case VK_SELECT:     return (U32)Platform::KeyCode::kKeySelect; 
		case VK_PRINT:      return (U32)Platform::KeyCode::kKeyPrint; 
		case VK_SNAPSHOT:   return (U32)Platform::KeyCode::kKeyPrintScreen; 
		case VK_INSERT:     return (U32)Platform::KeyCode::kKeyInsert; 
		case VK_DELETE:     return (U32)Platform::KeyCode::kKeyDelete; 
		case VK_HELP:       return (U32)Platform::KeyCode::kKeyHelp; 
		case VK_SLEEP:      return (U32)Platform::KeyCode::kKeySleep; 

		case VK_MULTIPLY:   return (U32)Platform::KeyCode::kKeyMultiply; 
		case VK_ADD:        return (U32)Platform::KeyCode::kKeyAdd; 
		case VK_SEPARATOR:  return (U32)Platform::KeyCode::kKeySeparator; 
		case VK_SUBTRACT:   return (U32)Platform::KeyCode::kKeySubtract; 
		case VK_DECIMAL:    return (U32)Platform::KeyCode::kKeyDecimal; 
		case VK_DIVIDE:     return (U32)Platform::KeyCode::kKeyDivide; 

		case VK_NUMLOCK:    return (U32)Platform::KeyCode::kKeyNumLock; 
		case VK_SCROLL:     return (U32)Platform::KeyCode::kKeyScrollLock; 
	}

	return (U32)Platform::KeyCode::kKeyInvalid;
}

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


void Platform::Init()
{
    // timing
    LARGE_INTEGER freq;
    BOOL res = ::QueryPerformanceFrequency(&freq);
    SM_ASSERT(res);
    s_timingFreqPerSec = freq.QuadPart;

    // dxc shader compiler
	SM_ASSERT(SUCCEEDED(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&s_dxcShaderCompilerLibrary))));
	SM_ASSERT(SUCCEEDED(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&s_dxcShaderCompiler))));
	SM_ASSERT(SUCCEEDED(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&s_dxcUtils))));
}

void Platform::Log(const char* format, ...)
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

static void HandleKeyDown(SM::Platform::KeyCode key)
{
    U8& keyState = s_keyStates[key];
    if(!IsBitSet(keyState, SM::Platform::kIsDown))
    {
        SetBit(keyState, SM::Platform::kWasPressed);
    }
    SetBit(keyState, SM::Platform::kIsDown);
}

static void HandleKeyUp(SM::Platform::KeyCode key)
{
    U8& keyState = s_keyStates[key];
    if(IsBitSet(keyState, SM::Platform::kIsDown))
    {
        SetBit(keyState, SM::Platform::kWasReleased);
    }
    UnSetBit(keyState, SM::Platform::kIsDown);
}

static LRESULT EngineWinProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;

	//bool imgui_handled_input = ImGui_ImplWin32_WndProcHandler(window_handle, msg, w_param, l_param);
    // if imgui_handled_input, don't modify input state

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
            Platform::Log("WM_CLOSE\n");
            Exit();
        }
        break;

        case WM_ACTIVATEAPP:
        {
            result = DefWindowProc(window, message, wParam, lParam);
        }
        break;

		case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        {
            // handle key down
            Platform::KeyCode key = (Platform::KeyCode)Win32KeyToEngineKey(wParam);
            HandleKeyDown(key);
        }
        break;

        case WM_LBUTTONDOWN:
        {
            HandleKeyDown(Platform::KeyCode::kMouseLButton);
        }
        break;

        case WM_MBUTTONDOWN:
        {
            HandleKeyDown(Platform::KeyCode::kMouseMButton);
        }
        break;

        case WM_RBUTTONDOWN: 
        {
            HandleKeyDown(Platform::KeyCode::kMouseRButton);
        }
        break;

		case WM_KEYUP:
        case WM_SYSKEYUP:
        {
            Platform::KeyCode key = (Platform::KeyCode)Win32KeyToEngineKey(wParam);
            HandleKeyUp(key);
        }

        case WM_LBUTTONUP:
        {
            HandleKeyUp(Platform::KeyCode::kMouseLButton);
        }
        break;

        case WM_MBUTTONUP:
        {
            HandleKeyUp(Platform::KeyCode::kMouseMButton);
        }
        break;

        case WM_RBUTTONUP:	
        {
            HandleKeyUp(Platform::KeyCode::kMouseRButton);
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

/*
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
*/

void Platform::Update(Window* pWindow)
{
    // Reset input state
	for (U32 i = 0; i < (U32)KeyCode::kNumKeyCodes; i++)
	{
		UnSetBit(s_keyStates[i], (U8)KeyStateBitFlags::kWasPressed | (U8)KeyStateBitFlags::kWasReleased);
	}

    // Update Mouse State?

    MSG msg;
    while (::PeekMessage(&msg, pWindow->m_hwnd, 0, 0, PM_REMOVE))
    {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
    }
}

void Platform::GetScreenDimensions(U32& screenWidth, U32& screenHeight)
{
    screenWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    screenHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
}

void Platform::GetWindowDimensions(Window* pWindow, U32& width, U32& height)
{
    RECT clientArea;
    BOOL success = ::GetClientRect(pWindow->m_hwnd, &clientArea);
    if(!success)
    {
        ReportLastWindowsError();
        width = 0;
        height = 0;
        return;
    }
    width = clientArea.right - clientArea.left;
    height = clientArea.bottom - clientArea.top;
}

bool Platform::IsWindowMinimized(Window* pWindow)
{
    return ::IsIconic(pWindow->m_hwnd); 
}

static VKAPI_ATTR VkBool32 VKAPI_CALL Win32VulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT msgSeverity,
													           VkDebugUtilsMessageTypeFlagsEXT msgType,
													           const VkDebugUtilsMessengerCallbackDataEXT* cbData,
													           void* userData)
{
	UNUSED(msgType);
	UNUSED(userData);

	// filter out verbose and info messages unless we explicitly want them
	if (!VulkanConfig::kEnableVerboseLog)
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

Shader* Platform::CompileShader(ShaderType shaderType, const char* shaderFile, const char* entryFunctionName, LinearAllocator* allocator)
{
    HRESULT hres;

    PushScopedStackAllocator(KiB(2));

    const char* fullFilepath = ConcatenateStrings(GetRawAssetsDir(), "Shaders\\");
    fullFilepath = ConcatenateStrings(fullFilepath, shaderFile);

    size_t fullFilepathLen = strlen(fullFilepath) + 1;
    wchar_t* fullFilepathW = SM::Alloc<wchar_t>(fullFilepathLen); 
	size_t numCharsConverted = 0;
	::mbstowcs_s(&numCharsConverted, fullFilepathW, fullFilepathLen, fullFilepath, fullFilepathLen);

    size_t entryFunctionNameLen = strlen(entryFunctionName) + 1;
    wchar_t* entryFunctionNameW = SM::Alloc<wchar_t>(entryFunctionNameLen); 
	numCharsConverted = 0;
	::mbstowcs_s(&numCharsConverted, entryFunctionNameW, entryFunctionNameLen, entryFunctionName, entryFunctionNameLen);

	// Load the HLSL text shader from disk
	U32 codePage = DXC_CP_ACP;
	CComPtr<IDxcBlobEncoding> sourceBlob;
	hres = s_dxcUtils->LoadFile(fullFilepathW, &codePage, &sourceBlob);
	SM_ASSERT(SUCCEEDED(hres));

	LPCWSTR targetProfile;
	switch (shaderType)
	{
        case SM::kVertex: targetProfile = L"vs66"; break;
        case SM::kPixel: targetProfile = L"ps66"; break;
		case SM::kCompute: targetProfile = L"cs66"; break;
        default: targetProfile = L"unknown"; break;
	}

	// configure the compiler arguments for compiling the HLSL shader to SPIR-V
    static const size_t kMaxNumArgs = 12;
    size_t numArgs = 0;
	LPCWSTR arguments[kMaxNumArgs];
    ::memset(arguments, 0, sizeof(LPCWSTR) * kMaxNumArgs);
	arguments[numArgs++] = (LPCWSTR)fullFilepathW;
	arguments[numArgs++] = L"-E";
	arguments[numArgs++] = (LPCWSTR)entryFunctionNameW;
	arguments[numArgs++] = L"-T";
	arguments[numArgs++] = targetProfile;
	arguments[numArgs++] = L"-spirv";

	if(IsRunningDebugBuild())
	{
        arguments[numArgs++] = L"-Zi";
        arguments[numArgs++] = L"-Od";
	}

	// Compile shader
	DxcBuffer buffer{};
	buffer.Encoding = DXC_CP_ACP;
	buffer.Ptr = sourceBlob->GetBufferPointer();
	buffer.Size = sourceBlob->GetBufferSize();

	CComPtr<IDxcResult> result{ nullptr };
	hres = s_dxcShaderCompiler->Compile(&buffer, arguments, numArgs, nullptr, IID_PPV_ARGS(&result));

	if (SUCCEEDED(hres)) 
	{
		result->GetStatus(&hres);
	}

	// Output error if compilation failed
	if (FAILED(hres) && (result)) 
	{
		CComPtr<IDxcBlobEncoding> errorBlob;
		hres = result->GetErrorBuffer(&errorBlob);
		if (SUCCEEDED(hres) && errorBlob) 
		{
			Log("Shader compilation failed for %s\n%s\n", shaderFile, (const char*)errorBlob->GetBufferPointer());
			return nullptr;
		}
	}

	// Get compilation result
	CComPtr<IDxcBlob> code;
	result->GetResult(&code);

    Shader* shader = allocator->Alloc<Shader>();
	shader->m_fileName = shaderFile;
	shader->m_entryFunctionName = entryFunctionName;
	shader->m_type = shaderType;
    shader->m_byteCode = (Byte*)allocator->Alloc(code->GetBufferSize());
    ::memcpy(shader->m_byteCode, code->GetBufferPointer(), code->GetBufferSize());
	return shader;
}

void Platform::ImguiInit(Window* pWindow, F32 fontSize)
{
    ImGui_ImplWin32_Init(pWindow->m_hwnd);

    F32 dpiScale = ImGui_ImplWin32_GetDpiScaleForHwnd(pWindow->m_hwnd);
    Platform::Log("[imgui-init] Setting ImGui DPI Scale to %f\n", dpiScale);

    ImFontConfig fontCfg;
    fontCfg.SizePixels = floor(fontSize * dpiScale);
    ImGui::GetIO().Fonts->AddFontDefault(&fontCfg);

    ImGui::GetStyle().ScaleAllSizes(dpiScale);
}

void Platform::ImguiBeginFrame()
{
    ImGui_ImplWin32_NewFrame();
}

F32 Platform::GetMillisecondsSinceAppStart()
{
    return GetSecondsSinceAppStart() * 1000.0f;
}

F32 Platform::GetSecondsSinceAppStart()
{
	LARGE_INTEGER perfCounter;
	SM_ASSERT(::QueryPerformanceCounter(&perfCounter));
	I64 curTick = perfCounter.QuadPart;
	F32 secondsElapsed = (F32)(curTick) / (F32)(s_timingFreqPerSec);
    return secondsElapsed;
}

void Platform::YieldThread()
{
	::SwitchToThread();
}

void Platform::SleepThreadSeconds(F32 seconds)
{
	SleepThreadMilliseconds(seconds * 1000.0f);
}

void Platform::SleepThreadMilliseconds(F32 ms)
{
	while (ms > 1000.0f)
	{
		::Sleep((DWORD)ms);
		ms -= 1000.0f;
	}

	while (ms > 100.0f)
	{
		::Sleep((DWORD)ms);
		ms -= 100.0f;
	}

	// spin down the last bit of time since calling the Win32 Sleep function is too inaccurate
    F32 msStart = GetMillisecondsSinceAppStart();
	while (GetMillisecondsSinceAppStart() - msStart < ms) { YieldThread(); }
}

bool Platform::ReadFileBytes(const char* filename, Byte*& outBytes, size_t& outNumBytes, LinearAllocator* allocator)
{
    HANDLE file = ::CreateFileA(filename, 
                                GENERIC_READ, 
                                FILE_SHARE_READ, 
                                NULL, 
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL, 
                                NULL);
    if(file == INVALID_HANDLE_VALUE)
    {
        ReportLastWindowsError();
        return false;
    }

    DWORD fileSize = ::GetFileSize(file, NULL);

    // allocate a buffer of size fileSize
    void* data = allocator->Alloc(fileSize);

    // read into it
    DWORD numBytesRead = 0;
    ::ReadFile(file, data, fileSize, &numBytesRead, NULL);

    SM_ASSERT(numBytesRead == fileSize);

    // close file
    ::CloseHandle(file);

    // return data
    outBytes = (Byte*)data;
    outNumBytes = numBytesRead;

    return true;
}

bool Platform::IsKeyDown(Platform::KeyCode key)
{
    return IsBitSet(s_keyStates[key], Platform::kIsDown);
}

bool Platform::WasKeyPressed(KeyCode key)
{
    return IsBitSet(s_keyStates[key], Platform::kWasPressed);
}

bool Platform::WasKeyReleased(KeyCode key)
{
    return IsBitSet(s_keyStates[key], Platform::kWasReleased);
}

void Platform::ShowMouse()
{
	::ShowCursor(true);
}

void Platform::HideMouse()
{
	::ShowCursor(false);
}

bool Platform::IsMouseShown()
{
    CURSORINFO cursorInfo;
    ::GetCursorInfo(&cursorInfo);
    return (cursorInfo.flags & CURSOR_SHOWING);
}

void Platform::GetMousePositionScreen(U32& xScreen, U32& yScreen)
{
	POINT mousePos;
	::GetCursorPos(&mousePos);
	xScreen = mousePos.x;
	yScreen = mousePos.y;
}

void Platform::SetMousePositionScreen(U32 xScreen, U32 yScreen)
{
	::SetCursorPos(xScreen, yScreen);
}

void Platform::GetMousePositionScreenNormalized(U32& xScreenNormalized, U32& yScreenNormalized)
{
    U32 xScreen = 0;
    U32 yScreen = 0;
    GetMousePositionScreen(xScreen, yScreen);

    U32 screenWidth = 0;
    U32 screenHeight = 0;
    GetScreenDimensions(screenWidth, screenHeight);

    xScreenNormalized = (F32)xScreen / (F32)screenWidth;
    yScreenNormalized = (F32)yScreen / (F32)screenHeight;
}

void Platform::SetMousePositionScreenNormalized(U32 xScreenNormalized, U32 yScreenNormalized)
{
    U32 screenWidth = 0;
    U32 screenHeight = 0;
    GetScreenDimensions(screenWidth, screenHeight);
    SetMousePositionScreen(xScreenNormalized * screenWidth, yScreenNormalized * screenHeight);
}

void Platform::GetMousePositionWindow(Window* pWindow, U32& xWindow, U32& yWindow)
{
    U32 xScreen = 0;
    U32 yScreen = 0;
    GetMousePositionScreen(xScreen, yScreen);

    POINT screenPos{.x = (LONG)xWindow, .y = (LONG)yWindow};
    ::ScreenToClient(pWindow->m_hwnd, &screenPos);

    xWindow = screenPos.x;
    yWindow = screenPos.y;
}

void Platform::SetMousePositionWindow(Window* pWindow, U32 xWindow, U32 yWindow)
{
    POINT screenPos{.x =  (LONG)xWindow, .y = (LONG)yWindow};
    ::ClientToScreen(pWindow->m_hwnd, &screenPos); 
    SetMousePositionScreen(screenPos.x, screenPos.y);
}

void Platform::GetMousePositionWindowNormalized(Window* pWindow, F32& xWindowNormalized, F32& yWindowNormalized)
{
    U32 xWindow = 0;
    U32 yWindow = 0;
    GetMousePositionWindow(pWindow, xWindow, yWindow);

    U32 windowWidth = 0;
    U32 windowHeight = 0;
    GetWindowDimensions(pWindow, windowWidth, windowHeight);

    xWindowNormalized = (F32)xWindow / (F32)windowWidth;
    yWindowNormalized = (F32)yWindow / (F32)windowHeight;
}

void Platform::SetMousePositionWindowNormalized(Window* pWindow, F32 xWindowNormalized, F32 yWindowNormalized)
{
    U32 windowWidth = 0;
    U32 windowHeight = 0;
    GetWindowDimensions(pWindow, windowWidth, windowHeight);
    
    U32 xWindow = xWindowNormalized * windowWidth;
    U32 yWindow = yWindowNormalized * windowHeight;

    POINT screenPos{ .x = (LONG)xWindow, .y = (LONG)yWindow };
    ::ClientToScreen(pWindow->m_hwnd, &screenPos);

    SetMousePositionScreen(screenPos.x, screenPos.y);
}
