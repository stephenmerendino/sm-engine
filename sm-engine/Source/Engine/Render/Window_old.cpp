#include "Engine/Render/Window_old.h"
#include "Engine/Core/Assert_old.h"
#include "Engine/Core/Debug_Old.h"
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
			::ShowWindow(pWindow->m_hwnd, SW_SHOW);
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

static void UpdateWindowSize(Window* pWindow)
{
	RECT size;
	::GetClientRect(pWindow->m_hwnd, &size);
	pWindow->m_width = size.right - size.left;
	pWindow->m_height = size.bottom - size.top;
}

static void UpdateWindowPosition(Window* pWindow)
{
	RECT pos;
	::GetWindowRect(pWindow->m_hwnd, &pos);
	pWindow->m_x = pos.left;
	pWindow->m_y = pos.top;
}

void Window::Init(const char* name, U32 width, U32 height, bool bResizable)
{
	SetProcessDPIAware(); // make sure window is created with scaling handled

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
	RECT fullSize;
	fullSize.left = 0;
	fullSize.top = 0;
	fullSize.right = width;
	fullSize.bottom = height;

	::AdjustWindowRectEx(&fullSize, style, FALSE, NULL);
	I32 createWidth = fullSize.right - fullSize.left;
	I32 createHeight = fullSize.bottom - fullSize.top;

	m_name = name;
	m_bWasResized = false;
	m_bWasClosed = false;
	m_bIsMinimized = false;

	wchar_t* unicodeName = AllocAndConvertUnicodeStr(name);
	FREE_AFTER_SCOPE(unicodeName);

	m_hwnd = CreateWindowEx(0,
						    WINDOW_CLASS_NAME,
						    unicodeName,
						    style,
						    CW_USEDEFAULT,
						    CW_USEDEFAULT,
						    createWidth,
						    createHeight,
						    NULL, NULL,
						    GetModuleHandle(NULL),
						    this);
	SM_ASSERT(NULL != m_hwnd);

	UpdateWindowSize(this);
	UpdateWindowPosition(this);

	// window adds a self subscription to catch windows messages for itself
	AddMsgCallback(InteralWindowMsgHandler, this);

	// make sure to show on init
	::ShowWindow(m_hwnd, SW_SHOW);
}

void Window::Destroy()
{
	::DestroyWindow(m_hwnd);
	::UnregisterClass(WINDOW_CLASS_NAME, ::GetModuleHandle(NULL));
}

void Window::Update()
{
	m_bWasResized = false;
	m_bIsMoving = false;

	// Pump messages
	MSG msg;
	while (::PeekMessage(&msg, m_hwnd, 0, 0, PM_REMOVE))
	{
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
	}

	UpdateWindowSize(this);
	UpdateWindowPosition(this);
	m_bIsMinimized = ::IsIconic(m_hwnd);
}

void Window::SetTitle(const char* newTitle)
{
	wchar_t* unicodeTitle = AllocAndConvertUnicodeStr(newTitle);
	FREE_AFTER_SCOPE(unicodeTitle);
	::SetWindowText(m_hwnd, unicodeTitle);
}

void Window::AddMsgCallback(WindowMsgCallbackFunc cb, void* userArgs)
{
	WindowMsgCallbackWithArgs cbWithArgs = { cb, userArgs };
	m_msgCallbacks.push_back(cbWithArgs);
}

void Window::RemoveMsgCallback(WindowMsgCallbackFunc cb, void* userArgs)
{		
	WindowMsgCallbackWithArgs cbWithArgs = { cb, userArgs };
	for (I32 i = 0; i < (I32)m_msgCallbacks.size(); i++)
	{
		if (m_msgCallbacks[i] == cbWithArgs)
		{
			m_msgCallbacks.erase(m_msgCallbacks.cbegin() + i);
		}
	}
}

IVec2 Window::CalcCenterPosition()
{
	U32 halfWidth = m_width / 2;
	U32 halfHeight = m_height / 2;
	return IVec2(m_x + halfWidth, m_y + halfHeight);
}

IVec2 Window::GetSize()
{
	return IVec2(m_width, m_height);
}