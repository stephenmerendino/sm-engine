#include "engine/render/Window.h"
#include "engine/core/Debug.h"
#include "engine/core/Assert.h"
#include "engine/core/macros.h"
#include <winuser.h>

static const char* WINDOW_CLASS_NAME = "App Window Class";

// message cb for all subscribers
static 
LRESULT main_window_msg_handler(HWND window_handle, UINT msg, WPARAM w_param, LPARAM l_param)
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

	LONG_PTR ptr = ::GetWindowLongPtr(window_handle, GWLP_USERDATA);
	window_t* window = (window_t*)ptr;

	if (window)
	{
		// Forward any OS messages to subscribers
		for (window_message_cb_with_args_t cb_with_args : window->m_message_cbs)
		{
			cb_with_args.m_cb(msg, w_param, l_param, cb_with_args.m_user_args);
		}
	}

	return ::DefWindowProc(window_handle, msg, w_param, l_param);
}

// internal message subscription for window
static 
void internal_window_msg_handler(UINT msg, WPARAM w_param, LPARAM l_param, void* user_args)
{
	UNUSED(l_param);

	window_t* window = (window_t*)user_args;

	switch (msg)
	{
        case WM_SIZE:
        {
            if (w_param == SIZE_RESTORED)
            {
                ::ShowWindow(window->m_handle, SW_SHOW);
            }
            else
            {
                window->m_was_resized = true;
            }
            break;
        }

        case WM_CLOSE:
        {
            window->m_was_closed = true;
        }

        default: break;
	}
}

static 
void window_update_size(window_t* window)
{
	RECT size;
	::GetClientRect(window->m_handle, &size);
	window->m_width = size.right - size.left;
	window->m_height = size.bottom - size.top;
}

static 
void window_update_position(window_t* window)
{
    RECT pos;
    ::GetWindowRect(window->m_handle, &pos);
    window->m_x = pos.left;
    window->m_y = pos.top;
}

window_t* window_create(const char* name, u32 width, u32 height, bool resizable)
{
	WNDCLASSEX wc = {};
	wc.cbSize = sizeof(wc);
	wc.style = CS_OWNDC | CS_DBLCLKS;
	wc.lpfnWndProc = (WNDPROC)main_window_msg_handler;
	wc.hInstance = ::GetModuleHandle(NULL);
	wc.lpszClassName = WINDOW_CLASS_NAME;
	RegisterClassEx(&wc);

	DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE;

	if (resizable)
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
	i32 create_width = full_size.right - full_size.left;
	i32 create_height = full_size.bottom - full_size.top;

    window_t* window = new window_t;
    MEM_ZERO(*window);
    window->m_name = name;
    window->m_was_resized = false;
    window->m_was_closed = false;
    window->m_is_minimized = false;

	window->m_handle = CreateWindowEx(0, 
                                      WINDOW_CLASS_NAME, 
                                      name, 
                                      style, 
                                      CW_USEDEFAULT, 
                                      CW_USEDEFAULT, 
                                      create_width, 
                                      create_height, 
                                      NULL, NULL, 
                                      GetModuleHandle(NULL), 
                                      window);
	ASSERT(NULL != window->m_handle);

    // verify that the created size is actually correct
    window_update_size(window);
    window_update_position(window);
    ASSERT(window->m_width == width && window->m_height == height);

    // window adds a self subscription to catch windows messages for itself
    window_add_msg_callback(window, internal_window_msg_handler, window);

	// make sure to show on init
	::ShowWindow(window->m_handle, SW_SHOW);

    return window;
}

void window_update(window_t* window)
{
    ASSERT(nullptr != window);

	window->m_was_resized = false;

	// Pump messages
	MSG msg;
	while (::PeekMessage(&msg, window->m_handle, 0, 0, PM_REMOVE))
	{
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
	}

    window_update_size(window);
    window_update_position(window);
	window->m_is_minimized = ::IsIconic(window->m_handle);
}

void window_set_title(window_t* window, const char* new_title)
{
    ASSERT(nullptr != window);
	LPCSTR new_title_win32 = (LPCSTR)(new_title);
	::SetWindowText(window->m_handle, new_title_win32);
}

void window_add_msg_callback(window_t* window, window_message_cb_t cb, void* args)
{
    ASSERT(nullptr != window);
	window_message_cb_with_args_t cb_with_args = { cb, args };
	window->m_message_cbs.push_back(cb_with_args);
}

ivec2 window_get_center_position(window_t* window)
{
    ASSERT(nullptr != window);
	u32 half_width = window->m_width / 2;
	u32 half_height = window->m_height / 2;
    return make_ivec2(window->m_x + half_width, window->m_y + half_height); 
}

void window_destroy(window_t* window)
{
    ASSERT(nullptr != window);
	::DestroyWindow(window->m_handle);
	::UnregisterClass(WINDOW_CLASS_NAME, ::GetModuleHandle(NULL));
    delete window;
}
