#include "sm/render/window.h"
#include "sm/core/assert.h"
#include "sm/core/debug.h"
#include "sm/core/string.h"
#include "sm/memory/arena.h"
#include "sm/platform/win32/win32_include.h"

using namespace sm;

static const wchar_t* WINDOW_CLASS_NAME = L"Window Class Name";
static const u8 kMaxNumCbs = 8;

window_msg_type_t translate_win32_msg(UINT msg)
{
	switch(msg)
	{
		case WM_CLOSE:		return window_msg_type_t::CLOSE_WINDOW;
		default:			return window_msg_type_t::UNKNOWN;
	}
}

u64 translate_win32_msg_data(UINT msg, WPARAM w_param, LPARAM l_param)
{
	return 0;
}

// message cb for all subscribers
static LRESULT win32_msg_handler(HWND window_handle, UINT msg, WPARAM w_param, LPARAM l_param)
{
	// Handle special case of creation so we can store the window pointer for later retrieval
	switch(msg)
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

	if(window)
	{
		// Translate the OS specific message to an agnostic engine defined message, pass that back to any cbs
		window_msg_type_t engine_msg = translate_win32_msg(msg);

		if(engine_msg != window_msg_type_t::UNKNOWN)
		{
            u64 engine_msg_data = translate_win32_msg_data(msg, w_param, l_param);
            for(int i = 0; i < window->num_cbs; i++)
            {
                const window_msg_cb_with_args_t& cb = window->msg_cbs[i];
                cb.cb(engine_msg, engine_msg_data, cb.args);
            }
		}
	}

	return ::DefWindowProc(window_handle, msg, w_param, l_param);
}

void internal_msg_handler(window_msg_type_t msg_type, u64 msg_data, void* user_args)
{
	sm::debug_printf("I received a message\n");
}

static void update_window_size(window_t* window)
{
	SM_ASSERT(window);
	HWND hwnd = sm::get_handle<HWND>(window->handle);

	RECT size;
	::GetClientRect(hwnd, &size);

	window->width = size.right - size.left;
	window->height = size.bottom - size.top;
}

static void update_window_position(window_t* window)
{
	SM_ASSERT(window);
	HWND hwnd = sm::get_handle<HWND>(window->handle);

	RECT pos;
	::GetWindowRect(hwnd, &pos);

	window->x = pos.left;
	window->y = pos.top;
}

window_t* sm::init_window(sm::arena_t* arena, const char* name, u32 width, u32 height, bool resizable)
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

	window_t* window = (window_t*)sm::alloc_zero(arena, sizeof(window_t));
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

	window->msg_cbs = init_static_array<window_msg_cb_with_args_t>(arena, kMaxNumCbs);
	window->num_cbs = 0;

	add_window_msg_cb(window, internal_msg_handler, &window);

	// make sure to show on init
	::ShowWindow(hwnd, SW_SHOW);

	return window;
}

void sm::add_window_msg_cb(window_t* window, window_msg_cb_t cb, void* args)
{
	SM_ASSERT(window->num_cbs < kMaxNumCbs);
	SM_ASSERT(window);
	window->msg_cbs[window->num_cbs] = { .cb = cb, .args = args };
	window->num_cbs++;
}

void sm::update_window(window_t* window)
{
	SM_ASSERT(window);
	window->was_resized = false;
	window->is_moving = false;

	// Pump messages
	HWND hwnd = get_handle<HWND>(window->handle);
	MSG msg;
	while (::PeekMessage(&msg, hwnd, 0, 0, PM_REMOVE))
	{
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
	}

	update_window_size(window);
	update_window_position(window);
	window->is_minimized = ::IsIconic(hwnd);
}

void sm::set_title(window_t* window, const char* new_title)
{
	SM_ASSERT(window);
	HWND hwnd = get_handle<HWND>(window->handle);
	static const u32 kMaxScratchpadSize = 256;
	wchar_t stack_scratchpad[kMaxScratchpadSize];
	sm::to_wchar_string(stack_scratchpad, kMaxScratchpadSize, new_title);
	::SetWindowText(hwnd, stack_scratchpad);
}
