#include "Engine/Render/Window.h"
#include "Engine/Core/Assert.h"
#include "Engine/Core/Macros.h"
#include "Engine/Platform/WindowsUtils.h"
#include <cstdlib>

static const wchar_t* WINDOW_CLASS_NAME = L"App Window Class";

// message cb for all subscribers
static LRESULT MainWindowMsgHandler(HWND window_handle, UINT msg, WPARAM w_param, LPARAM l_param)
{
	// Handle special case of creation so we can store the window pointer for later retrieval
	switch (msg)
	{
	case WM_CREATE:
	{
		CREATESTRUCT* cs = (CREATESTRUCT*)l_param;
		Window* pWindow = (Window*)cs->lpCreateParams;
		::SetWindowLongPtr(window_handle, GWLP_USERDATA, (LONG_PTR)pWindow);
		return ::DefWindowProc(window_handle, msg, w_param, l_param);
	}
	}

	LONG_PTR ptr = ::GetWindowLongPtr(window_handle, GWLP_USERDATA);
	Window* pWindow = (Window*)ptr;

	if (pWindow)
	{
		// Forward any OS messages to subscribers
		for (WindowMsgCallbackWithArgs cbWithArgs : pWindow->m_msgCallbacks)
		{
			cbWithArgs.m_cb(msg, w_param, l_param, cbWithArgs.m_userArgs);
		}
	}

	return ::DefWindowProc(window_handle, msg, w_param, l_param);
}

static void InteralWindowMsgHandler(UINT msg, WPARAM wParam, LPARAM lParam, void* userArgs)
{
	UNUSED(lParam);

	Window* pWindow = (Window*)userArgs;

	switch (msg)
	{
	case WM_SIZE:
	{
		if (wParam == SIZE_RESTORED)
		{
			::ShowWindow(pWindow->m_handle, SW_SHOW);
		}
		else
		{
			pWindow->m_bWasResized = true;
		}
		break;
	}

	case WM_MOVE:
	{
		pWindow->m_bIsMoving = true;
		break;
	}

	case WM_CLOSE:
	{
		pWindow->m_bWasClosed = true;
		break;
	}

	default: break;
	}
}

bool Window::Init(const char* name, U32 width, U32 height, bool bResizable)
{
	WNDCLASSEX wc = {};
	wc.cbSize = sizeof(wc);
	wc.style = CS_OWNDC | CS_DBLCLKS;
	wc.lpfnWndProc = (WNDPROC)MainWindowMsgHandler;
	wc.hInstance = ::GetModuleHandle(NULL);
	wc.lpszClassName = WINDOW_CLASS_NAME;
	wc.hCursor = ::LoadCursor(NULL, IDC_ARROW);
	RegisterClassEx(&wc);

	DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE;

	if (bResizable)
	{
		style |= WS_THICKFRAME;
	}

	// calculate how big to create the window to ensure the "client area" (renderable surface) is actually sized to requested width & height
	RECT full_size;
	full_size.left = 0;
	full_size.top = 0;
	full_size.right = width;
	full_size.bottom = height;

	::AdjustWindowRectEx(&full_size, style, FALSE, NULL);
	I32 create_width = full_size.right - full_size.left;
	I32 create_height = full_size.bottom - full_size.top;

	Window* pWindow = new Window;
	MEM_ZERO(*pWindow);
	pWindow->m_name = name;
	pWindow->m_bWasResized = false;
	pWindow->m_bWasClosed = false;
	pWindow->m_bIsMinimized = false;

	wchar_t* unicodeName = AllocAndConvertUnicodeStr(name);
	FREE_AFTER_SCOPE(unicodeName);

	pWindow->m_handle = CreateWindowEx(0,
									   WINDOW_CLASS_NAME,
									   unicodeName,
									   style,
									   CW_USEDEFAULT,
									   CW_USEDEFAULT,
									   create_width,
									   create_height,
									   NULL, NULL,
									   GetModuleHandle(NULL),
									   pWindow);
	SM_ASSERT(NULL != pWindow->m_handle);

	// verify that the created size is actually correct
	//window_update_size(window);
	//window_update_position(window);
	//SM_ASSERT(window->width == width && window->height == height);

	// window adds a self subscription to catch windows messages for itself
	//window_add_msg_callback(window, internal_window_msg_handler, window);

	// make sure to show on init
	::ShowWindow(pWindow->m_handle, SW_SHOW);

	return true;
}