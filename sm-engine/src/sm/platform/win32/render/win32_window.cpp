#include "sm/render/window.h"
#include "sm/core/assert.h"
#include "sm/core/bits.h"
#include "sm/core/debug.h"
#include "sm/core/string.h"
#include "sm/io/input.h"
#include "sm/memory/arena.h"
#include "sm/platform/win32/win32_include.h"
#include "third_party/imgui/imgui.h"

using namespace sm;

static const wchar_t* WINDOW_CLASS_NAME = L"Window Class Name";
static const u8 kMaxNumCbs = 8;

static i32 win32_key_to_engine_key(u32 windows_key)
{
	// https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes

	// Numbers 0-9
	// 0x30 = 0, 0x39 = 9
	if (windows_key >= 0x30 && windows_key <= 0x39)
	{
		return (u32)key_code_t::KEY_0 + (windows_key - 0x30);
	}

	// Letters A-Z
	// 0x41 = A, 0x5A = Z
	if (windows_key >= 0x41 && windows_key <= 0x5A)
	{
		return (u32)key_code_t::KEY_A + (windows_key - 0x41);
	}

	// Numpad 0-9
	if (windows_key >= VK_NUMPAD0 && windows_key <= VK_NUMPAD9)
	{
		return (u32)key_code_t::KEY_NUMPAD0 + (windows_key - VK_NUMPAD0);
	}

	// F1-F24
	if (windows_key >= VK_F1 && windows_key <= VK_F24)
	{
		return (u32)key_code_t::KEY_F1 + (windows_key - VK_F1);
	}

	// Handle everything else directly
	switch (windows_key)
	{
		case VK_LBUTTON:    return (u32)key_code_t::MOUSE_LBUTTON; 
		case VK_RBUTTON:    return (u32)key_code_t::MOUSE_RBUTTON; 
		case VK_MBUTTON:    return (u32)key_code_t::MOUSE_MBUTTON; 

		case VK_BACK:       return (u32)key_code_t::KEY_BACKSPACE; 
		case VK_TAB:        return (u32)key_code_t::KEY_TAB; 
		case VK_CLEAR:      return (u32)key_code_t::KEY_CLEAR; 
		case VK_RETURN:     return (u32)key_code_t::KEY_ENTER; 
		case VK_SHIFT:      return (u32)key_code_t::KEY_SHIFT; 
		case VK_CONTROL:    return (u32)key_code_t::KEY_CONTROL; 
		case VK_MENU:       return (u32)key_code_t::KEY_ALT; 
		case VK_PAUSE:      return (u32)key_code_t::KEY_PAUSE; 
		case VK_CAPITAL:    return (u32)key_code_t::KEY_CAPSLOCK; 
		case VK_ESCAPE:     return (u32)key_code_t::KEY_ESCAPE; 
		case VK_SPACE:      return (u32)key_code_t::KEY_SPACE; 
		case VK_PRIOR:      return (u32)key_code_t::KEY_PAGEUP; 
		case VK_NEXT:       return (u32)key_code_t::KEY_PAGEDOWN; 
		case VK_END:        return (u32)key_code_t::KEY_END; 
		case VK_HOME:       return (u32)key_code_t::KEY_HOME; 
		case VK_LEFT:       return (u32)key_code_t::KEY_LEFTARROW; 
		case VK_UP:         return (u32)key_code_t::KEY_UPARROW; 
		case VK_RIGHT:      return (u32)key_code_t::KEY_RIGHTARROW; 
		case VK_DOWN:       return (u32)key_code_t::KEY_DOWNARROW; 
		case VK_SELECT:     return (u32)key_code_t::KEY_SELECT; 
		case VK_PRINT:      return (u32)key_code_t::KEY_PRINT; 
		case VK_SNAPSHOT:   return (u32)key_code_t::KEY_PRINTSCREEN; 
		case VK_INSERT:     return (u32)key_code_t::KEY_INSERT; 
		case VK_DELETE:     return (u32)key_code_t::KEY_DELETE; 
		case VK_HELP:       return (u32)key_code_t::KEY_HELP; 
		case VK_SLEEP:      return (u32)key_code_t::KEY_SLEEP; 

		case VK_MULTIPLY:   return (u32)key_code_t::KEY_MULTIPLY; 
		case VK_ADD:        return (u32)key_code_t::KEY_ADD; 
		case VK_SEPARATOR:  return (u32)key_code_t::KEY_SEPARATOR; 
		case VK_SUBTRACT:   return (u32)key_code_t::KEY_SUBTRACT; 
		case VK_DECIMAL:    return (u32)key_code_t::KEY_DECIMAL; 
		case VK_DIVIDE:     return (u32)key_code_t::KEY_DIVIDE; 

		case VK_NUMLOCK:    return (u32)key_code_t::KEY_NUMLOCK; 
		case VK_SCROLL:     return (u32)key_code_t::KEY_SCROLLLOCK; 
	}

	return (u32)key_code_t::KEY_INVALID;
}

window_msg_type_t translate_win32_msg(UINT msg)
{
	switch(msg)
	{
		case WM_CLOSE: 
			return window_msg_type_t::CLOSE_WINDOW;

		case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN: 
			return window_msg_type_t::KEY_DOWN;

		case WM_KEYUP:
        case WM_SYSKEYUP:
        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP:	
			return window_msg_type_t::KEY_UP;

		default: 
			return window_msg_type_t::UNKNOWN;
	}
}

u64 translate_win32_msg_data(UINT msg, WPARAM w_param, LPARAM l_param)
{
	switch(msg)
	{
		case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
			return win32_key_to_engine_key((u32)w_param);
        case WM_LBUTTONDOWN:
			return win32_key_to_engine_key(VK_LBUTTON);
        case WM_MBUTTONDOWN:
			return win32_key_to_engine_key(VK_MBUTTON);
        case WM_RBUTTONDOWN: 
			return win32_key_to_engine_key(VK_RBUTTON);

		case WM_KEYUP:
        case WM_SYSKEYUP:
			return win32_key_to_engine_key((u32)w_param);
        case WM_LBUTTONUP:
			return win32_key_to_engine_key(VK_LBUTTON);
        case WM_MBUTTONUP:
			return win32_key_to_engine_key(VK_MBUTTON);
        case WM_RBUTTONUP: 
			return win32_key_to_engine_key(VK_RBUTTON);

		default: 
			return 0;
	}
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

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

	bool imgui_handled_input = ImGui_ImplWin32_WndProcHandler(window_handle, msg, w_param, l_param);

	if(window)
	{
		// Translate the OS specific message to an agnostic engine defined message, pass that back to any cbs
		window_msg_type_t engine_msg = translate_win32_msg(msg);

		if(engine_msg != window_msg_type_t::UNKNOWN)
		{
            u64 engine_msg_data = translate_win32_msg_data(msg, w_param, l_param);
			if(imgui_handled_input)
			{
				engine_msg = window_msg_type_t::IMGUI_HANDLED_INPUT;
			}

            for(int i = 0; i < window->msg_cbs.cur_size; i++)
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

window_t* sm::window_init(sm::arena_t* arena, const char* name, u32 width, u32 height, bool resizable)
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

	window_t* window = (window_t*)sm::arena_alloc_zero(arena, sizeof(window_t));
	window->name = name;
	window->was_resized = false;
	window->was_closed = false;
	window->is_minimized = false;
	window->is_moving = false;

	wchar_t* unicode_name = sm::string_to_wchar(arena, name);

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

	window->msg_cbs = array_init<window_msg_cb_with_args_t>(arena, kMaxNumCbs);

	window_add_msg_cb(window, internal_msg_handler, &window);

	// make sure to show on init
	::ShowWindow(hwnd, SW_SHOW);
	::BringWindowToTop(hwnd);

	return window;
}

void sm::window_add_msg_cb(window_t* window, window_msg_cb_t cb, void* args)
{
	SM_ASSERT(window);
	window_msg_cb_with_args_t cb_with_args{};
	cb_with_args.cb = cb;
	cb_with_args.args = args;
	array_push(window->msg_cbs, cb_with_args);
}

void sm::window_update(window_t* window)
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

void sm::window_set_title(window_t* window, const char* new_title)
{
	SM_ASSERT(window);
	HWND hwnd = get_handle<HWND>(window->handle);
	static const u32 kMaxScratchpadSize = 256;
	wchar_t stack_scratchpad[kMaxScratchpadSize];
	sm::string_to_wchar(stack_scratchpad, kMaxScratchpadSize, new_title);
	::SetWindowText(hwnd, stack_scratchpad);
}

bool sm::window_is_minimized(const window_t* window)
{
	SM_ASSERT(window);
	HWND hwnd = get_handle<HWND>(window->handle);
	return ::IsIconic(hwnd);
}
