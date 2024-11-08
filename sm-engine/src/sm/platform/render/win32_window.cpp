#include "sm/render/window.h"
#include "sm/core/assert.h"
#include "sm/core/string.h"
#include "sm/memory/arena.h"
#include "sm/platform/win32/win32_include.h"

using namespace sm::render;

static const wchar_t* WINDOW_CLASS_NAME = L"Window Class Name";

// message cb for all subscribers
static LRESULT win32_msg_handler(HWND window_handle, UINT msg, WPARAM w_param, LPARAM l_param)
{
	// Handle special case of creation so we can store the window pointer for later retrieval
	switch (msg)
	{
	case WM_CREATE:
	{
		CREATESTRUCT* cs = (CREATESTRUCT*)l_param;
		window_t* window = (window_t*)cs->lpCreateParams;
		::SetWindowLongPtr(window_handle, GWLP_USERDATA, (LONG_PTR)window);
		return ::DefWindowProc(window_handle, msg, w_param, l_param);
	}
	}

	//LONG_PTR ptr = ::GetWindowLongPtr(window_handle, GWLP_USERDATA);
	//Window* pWindow = (Window*)ptr;

	//if (pWindow)
	//{
	//	// Forward any OS messages to subscribers
	//	for (WindowMsgCallbackWithArgs cbWithArgs : pWindow->m_msgCallbacks)
	//	{
	//		cbWithArgs.m_cb(msg, w_param, l_param, cbWithArgs.m_userArgs);
	//	}
	//}

	return ::DefWindowProc(window_handle, msg, w_param, l_param);
}

static void update_window_size(window_t* window)
{
	HWND hwnd = sm::get_handle<HWND>(window->handle);

	RECT size;
	::GetClientRect(hwnd, &size);

	window->width = size.right - size.left;
	window->height = size.bottom - size.top;
}

static void update_window_position(window_t* window)
{
	HWND hwnd = sm::get_handle<HWND>(window->handle);

	RECT pos;
	::GetWindowRect(hwnd, &pos);

	window->x = pos.left;
	window->y = pos.top;
}

window_t* sm::render::init_window(sm::memory::arena_t* arena, const char* name, u32 width, u32 height, bool resizable)
{
	SetProcessDPIAware(); // make sure window is created with scaling handled

	WNDCLASSEX wc = {};
	wc.cbSize = sizeof(wc);
	wc.style = CS_OWNDC | CS_DBLCLKS;
	wc.lpfnWndProc = (WNDPROC)win32_msg_handler;
	wc.hInstance = ::GetModuleHandle(NULL);
	wc.lpszClassName = WINDOW_CLASS_NAME;
	wc.hCursor = ::LoadCursor(NULL, IDC_ARROW);
	RegisterClassEx(&wc);

	DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE;
	if (resizable)
	{
		style |= WS_THICKFRAME;
	}

	// calculate how big to create the window to ensure the "client area" (renderable surface) is actually sized to width & height params
	RECT full_size;
	full_size.left = 0;
	full_size.top = 0;
	full_size.right = width;
	full_size.bottom = height;

	::AdjustWindowRectEx(&full_size, style, FALSE, NULL);
	i32 create_width = full_size.right - full_size.left;
	i32 create_height = full_size.bottom - full_size.top;

	window_t* window = (window_t*)sm::memory::alloc_zero(arena, sizeof(window_t));
	window->name = name;
	window->was_resized = false;
	window->was_closed = false;
	window->is_minimized = false;
	window->is_moving = false;

    wchar_t* unicode_name = sm::to_wchar_string(arena, name);

	HWND hwnd = CreateWindowEx(0,
						    WINDOW_CLASS_NAME,
						    unicode_name,
						    style,
						    CW_USEDEFAULT,
						    CW_USEDEFAULT,
						    create_width,
						    create_height,
						    NULL, NULL,
						    GetModuleHandle(NULL),
						    window); // used to pass this pointer

	SM_ASSERT(NULL != hwnd);
	set_handle(window->handle, hwnd);

	update_window_size(window);
	update_window_position(window);

	// window adds a self subscription to catch windows messages for itself
	//AddMsgCallback(InteralWindowMsgHandler, this);

	// make sure to show on init
	::ShowWindow(hwnd, SW_SHOW);

	return window;
}