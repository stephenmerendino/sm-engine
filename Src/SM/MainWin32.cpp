#include "SM/Engine.h"
#include "SM/Assert.h"
#include "SM/Platform.h"

#include <cstdio>
#include <cstdlib>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

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

bool SM::Platform::AssertReportFailure(const char* expression, const char* filename, int lineNumber)
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

bool SM::Platform::AssertReportFailureMsg(const char* expression, const char* msg, const char* filename, int lineNumber)
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

bool SM::Platform::AssertReportError(const char* filename, int lineNumber)
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

bool SM::Platform::AssertReportErrorMsg(const char* msg, const char* filename, int lineNumber)
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
    GameLoadedFunction GameLoaded = nullptr;
    GameUpdateFunction GameUpdate = nullptr;
    GameRenderFunction GameRender = nullptr;
    FILETIME m_dllLastWriteTime;
    bool m_bIsLoaded = false;
};

static bool LoadGameCode(const char* dllFilename, GameApi& gameApi)
{
    if(!gameApi.m_bIsLoaded)
    {
        gameApi.GameLoaded = [](SM::EngineApi engineApi){ };
        gameApi.GameUpdate = [](){ };
        gameApi.GameRender = [](){ };
    }

    WIN32_FILE_ATTRIBUTE_DATA fileInfo = {};
    bool gotFileInfo = ::GetFileAttributesExA(dllFilename, GetFileExInfoStandard, &fileInfo);
    if(!gotFileInfo)
    {
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

    GameLoadedFunction GameLoaded = (GameLoadedFunction)::GetProcAddress(gameDLL, GAME_LOADED_FUNCTION_NAME_STRING);
    if(GameLoaded == NULL)
    {
        Platform::Log("Couldn't find %s in DLL\n", GAME_LOADED_FUNCTION_NAME_STRING);
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

    gameApi.GameLoaded = GameLoaded;
    gameApi.GameUpdate = GameUpdate;
    gameApi.GameRender = GameRender;
    gameApi.m_dllLastWriteTime = fileInfo.ftLastWriteTime;

    EngineApi engineApi = {
        .EngineLog = &Platform::Log
    };
    gameApi.GameLoaded(engineApi);

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
	window.m_hwnd = CreateWindowExA(0,
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
	SM_ASSERT(NULL != window.m_hwnd);

	// make sure to show on init
	::ShowWindow(window.m_hwnd, SW_SHOW);
	::BringWindowToTop(window.m_hwnd);

    return window;
}

int WINAPI WinMain(HINSTANCE app, 
				   HINSTANCE prevApp, 
				   LPSTR args, 
				   int show)
{
    using namespace SM;

    const char* dllName = "Workbench.dll";

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
