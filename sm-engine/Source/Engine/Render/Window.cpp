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

static void UpdateWindowSize(Window* pWindow)
{
	RECT size;
	::GetClientRect(pWindow->m_handle, &size);
	pWindow->m_width = size.right - size.left;
	pWindow->m_height = size.bottom - size.top;
}

static void UpdateWindowPosition(Window* pWindow)
{
	RECT pos;
	::GetWindowRect(pWindow->m_handle, &pos);
	pWindow->m_x = pos.left;
	pWindow->m_y = pos.top;
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

	m_name = name;
	m_bWasResized = false;
	m_bWasClosed = false;
	m_bIsMinimized = false;

	wchar_t* unicodeName = AllocAndConvertUnicodeStr(name);
	FREE_AFTER_SCOPE(unicodeName);

	m_handle = CreateWindowEx(0,
						      WINDOW_CLASS_NAME,
						      unicodeName,
						      style,
						      CW_USEDEFAULT,
						      CW_USEDEFAULT,
						      create_width,
						      create_height,
						      NULL, NULL,
						      GetModuleHandle(NULL),
						      this);
	SM_ASSERT(NULL != m_handle);

	UpdateWindowSize(this);
	UpdateWindowPosition(this);

	// window adds a self subscription to catch windows messages for itself
	AddMsgCallback(InteralWindowMsgHandler, this);

	// make sure to show on init
	::ShowWindow(m_handle, SW_SHOW);

	return true;
}

void Window::Destroy()
{
	::DestroyWindow(m_handle);
	::UnregisterClass(WINDOW_CLASS_NAME, ::GetModuleHandle(NULL));
}

void Window::Update()
{
	m_bWasResized = false;
	m_bIsMoving = false;

	// Pump messages
	MSG msg;
	while (::PeekMessage(&msg, m_handle, 0, 0, PM_REMOVE))
	{
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
	}

	UpdateWindowSize(this);
	UpdateWindowPosition(this);
	m_bIsMinimized = ::IsIconic(m_handle);
}

void Window::SetTitle(const char* newTitle)
{
	wchar_t* unicodeTitle = AllocAndConvertUnicodeStr(newTitle);
	FREE_AFTER_SCOPE(unicodeTitle);
	::SetWindowText(m_handle, unicodeTitle);
}

void Window::AddMsgCallback(WindowMsgCallbackFunc cb, void* userArgs)
{
	WindowMsgCallbackWithArgs cbWithArgs = { cb, userArgs };
	m_msgCallbacks.push_back(cbWithArgs);
}

IVec2 Window::CalcCenterPosition()
{
	U32 halfWidth = m_width / 2;
	U32 halfHeight = m_height / 2;
	return IVec2(m_x + halfWidth, m_y + halfHeight);
}