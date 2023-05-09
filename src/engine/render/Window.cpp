#include "engine/render/Window.h"
#include "engine/core/Debug.h"
#include "engine/core/Assert.h"
#include "engine/core/macros.h"

static const char* WINDOW_CLASS_NAME = "App Window Class";

// message cb for all subscribers
static LRESULT main_window_msg_handler(HWND window_handle, UINT msg, WPARAM w_param, LPARAM l_param)
{
	// Handle special case of creation so we can store the window pointer for later retrieval
	switch (msg)
	{
        case WM_CREATE:
        {
            CREATESTRUCT* cs = (CREATESTRUCT*)l_param;
            window_t* p_window = (window_t*)cs->lpCreateParams;
            ::SetWindowLongPtr(window_handle, GWLP_USERDATA, (LONG_PTR)p_window);
            return ::DefWindowProc(window_handle, msg, w_param, l_param);
        }
	}

	LONG_PTR ptr = ::GetWindowLongPtr(window_handle, GWLP_USERDATA);
	window_t* p_window = (window_t*)ptr;

	if (p_window)
	{
		// Forward any OS messages to subscribers
		for (window_message_cb_with_args_t cb_with_args : p_window->m_message_cbs)
		{
			cb_with_args.m_cb(msg, w_param, l_param, cb_with_args.m_user_args);
		}
	}

	return ::DefWindowProc(window_handle, msg, w_param, l_param);
}

// internal message subscription for window
static void internal_window_msg_handler(UINT msg, WPARAM w_param, LPARAM l_param, void* user_args)
{
	UNUSED(l_param);

	window_t* p_window = (window_t*)user_args;

	switch (msg)
	{
        case WM_SIZE:
        {
            if (w_param == SIZE_RESTORED)
            {
                ::ShowWindow(p_window->m_handle, SW_SHOW);
            }
            else
            {
                p_window->m_was_resized = true;
            }
            break;
        }

        default: break;
	}
}

static void update_window_size(window_t* p_window)
{
	RECT size;
	::GetClientRect(p_window->m_handle, &size);
	p_window->m_width = size.right - size.left;
	p_window->m_height = size.bottom - size.top;
}

window_t* create_window(const char* name, u32 width, u32 height, bool resizable)
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

    window_t* p_window = new window_t;
    MEM_ZERO(*p_window);
    p_window->m_name = name;
    p_window->m_was_resized = false;
    p_window->m_is_minimized = false;

	p_window->m_handle = CreateWindowEx(0, 
                                        WINDOW_CLASS_NAME, 
                                        name, 
                                        style, 
                                        CW_USEDEFAULT, 
                                        CW_USEDEFAULT, 
                                        create_width, 
                                        create_height, 
                                        NULL, NULL, 
                                        GetModuleHandle(NULL), 
                                        p_window);
	ASSERT(NULL != p_window->m_handle);

    // verify that the created size is actually correct
    update_window_size(p_window);
    ASSERT(p_window->m_width == width && p_window->m_height == height);

    // window adds a self subscription to catch windows messages for itself
    add_window_callback(p_window, internal_window_msg_handler, p_window);

	// make sure to show on init
	::ShowWindow(p_window->m_handle, SW_SHOW);

    return p_window;
}

void update_window(window_t* window)
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

    update_window_size(window);
	window->m_is_minimized = ::IsIconic(window->m_handle);
}

void set_window_title(window_t* window, const char* new_title)
{
    ASSERT(nullptr != window);
	LPCSTR new_title_win32 = (LPCSTR)(new_title);
	::SetWindowText(window->m_handle, new_title_win32);
}

void add_window_callback(window_t* window, window_message_cb_t cb, void* args)
{
	window_message_cb_with_args_t cb_with_args = { cb, args };
	window->m_message_cbs.push_back(cb_with_args);
}

void destroy_window(window_t* window)
{
    ASSERT(nullptr != window);
	::DestroyWindow(window->m_handle);
	::UnregisterClass(WINDOW_CLASS_NAME, ::GetModuleHandle(NULL));
    delete window;
}
