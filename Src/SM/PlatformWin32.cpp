#include "SM/Platform.h"
#include "SM/Assert.h"
#include "SM/Engine.h"
#include "SM/Memory.h"

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
CComPtr<IDxcCompiler3> s_dcxShaderCompiler;
CComPtr<IDxcUtils> s_dcxUtils;

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


void Platform::Init()
{
    // timing
    LARGE_INTEGER freq;
    BOOL res = ::QueryPerformanceFrequency(&freq);
    SM_ASSERT(res);
    s_timingFreqPerSec = freq.QuadPart;

    // dxc shader compiler
	SM_ASSERT(SUCCEEDED(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&s_dxcShaderCompilerLibrary))));
	SM_ASSERT(SUCCEEDED(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&s_dcxShaderCompiler))));
	SM_ASSERT(SUCCEEDED(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&s_dcxUtils))));
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

void Platform::CompileShader()
{
	//HRESULT hres;

	//// append on the root shader directory
	//// todo: move this out of platform specific code
	//string_t full_filepath = string_init(arena);
	//string_append(full_filepath, SHADERS_PATH);
	//string_append(full_filepath, file_name);

	//// need to convert filepath from const char * to LPCWSTR
	//wchar_t* full_filepath_w = string_to_wchar(arena, full_filepath);
	//wchar_t* entry_name_w = string_to_wchar(arena, entry_name);

	//// Load the HLSL text shader from disk
	//uint32_t code_page = DXC_CP_ACP;
	//CComPtr<IDxcBlobEncoding> source_blob;
	//hres = s_utils->LoadFile(full_filepath_w, &code_page, &source_blob);
	//SM_ASSERT(SUCCEEDED(hres));

	//LPCWSTR target_profile;
	//switch (shader_type)
	//{
    //    case shader_type_t::VERTEX: target_profile = L"vs_6_6"; break;
    //    case shader_type_t::PIXEL: target_profile = L"ps_6_6"; break;
	//	case shader_type_t::COMPUTE: target_profile = L"cs_6_6"; break;
    //    default: target_profile = L"unknown"; break;
	//}

	//// configure the compiler arguments for compiling the HLSL shader to SPIR-V
	//array_t<LPCWSTR> arguments = array_init<LPCWSTR>(arena, 8);
	//array_push(arguments, (LPCWSTR)full_filepath_w);
	//array_push(arguments, L"-E");
	//array_push(arguments, (LPCWSTR)entry_name_w);
	//array_push(arguments, L"-T");
	//array_push(arguments, target_profile);
	//array_push(arguments, L"-spirv");

	//if(is_running_in_debug())
	//{
    //    array_push(arguments, L"-Zi");
    //    array_push(arguments, L"-Od");
	//}

	//// Compile shader
	//DxcBuffer buffer{};
	//buffer.Encoding = DXC_CP_ACP;
	//buffer.Ptr = source_blob->GetBufferPointer();
	//buffer.Size = source_blob->GetBufferSize();

	//CComPtr<IDxcResult> result{ nullptr };
	//hres = s_compiler->Compile(&buffer, arguments.data, (uint32_t)arguments.cur_size, nullptr, IID_PPV_ARGS(&result));

	//if (SUCCEEDED(hres)) 
	//{
	//	result->GetStatus(&hres);
	//}

	//// Output error if compilation failed
	//if (FAILED(hres) && (result)) 
	//{
	//	CComPtr<IDxcBlobEncoding> error_blob;
	//	hres = result->GetErrorBuffer(&error_blob);
	//	if (SUCCEEDED(hres) && error_blob) 
	//	{
	//		debug_printf("Shader compilation failed for %s\n%s\n", file_name, (const char*)error_blob->GetBufferPointer());
	//		return false;
	//	}
	//}

	//// Get compilation result
	//CComPtr<IDxcBlob> code;
	//result->GetResult(&code);

	//(*out_shader)->file_name = file_name;
	//(*out_shader)->entry_name = entry_name;
	//(*out_shader)->shader_type = shader_type;

	//(*out_shader)->bytecode = array_init<byte_t>(arena, code->GetBufferSize());
	//array_push((*out_shader)->bytecode, (byte_t*)code->GetBufferPointer(), code->GetBufferSize());

	//return true;
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

bool Platform::LoadFileBytes(const char* filename, Byte* outBytes, size_t outNumBytes)
{
	//array_t<byte_t> data;

	//// open file for read and binary
	//FILE* p_file = NULL;
	//fopen_s(&p_file, filename, "rb");
	//if (NULL == p_file)
	//{
	//	debug_printf("Failed to open file [%s] to read bytes\n", filename);
	//	return data;
	//}

	//// measure size
	//fseek(p_file, 0, SEEK_END);
	//long file_len = ftell(p_file);
	//rewind(p_file);

	//// alloc buffer
	//data = array_init_sized<byte_t>(arena, file_len);

	//// read into buffer
	//size_t bytes_read = fread(data.data, sizeof(byte_t), file_len, p_file);
	//SM_ASSERT(bytes_read == (size_t)file_len);

	//// close file
	//fclose(p_file);

	//// return buffer
	//return data;

    return false;
}
