#include "engine/render/Window.h"
#include "engine/core/Debug.h"
#include "engine/core/Common.h"
#include "engine/core/Assert.h"

static const char* WINDOW_CLASS_NAME = "App Window Class";

LRESULT MainWindowMsgHandler(HWND hWindow, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// Handle special case of creation so we can store the window pointer for later retrieval
	switch (msg)
	{
	case WM_CREATE:
	{
		CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
		Window* pAppWindow = (Window*)cs->lpCreateParams;
		::SetWindowLongPtr(hWindow, GWLP_USERDATA, (LONG_PTR)pAppWindow);
		return ::DefWindowProc(hWindow, msg, wParam, lParam);
	}
	}

	LONG_PTR ptr = ::GetWindowLongPtr(hWindow, GWLP_USERDATA);
	Window* pMainWindow = (Window*)ptr;

	if (pMainWindow)
	{
		// Forward any OS messages to subscribers
		for (WindowMessageCbWithArgs msgCbWithArgs : pMainWindow->GetMessageCallbacks())
		{
			msgCbWithArgs.m_msgCb(msg, wParam, lParam, msgCbWithArgs.m_userArgs);
		}
	}

	return ::DefWindowProc(hWindow, msg, wParam, lParam);
}

static void InternalWindowMessageHandler(UINT msg, WPARAM wParam, LPARAM lParam, void* userArgs)
{
	Unused(lParam);
	Window* pWindow = (Window*)userArgs;

	switch (msg)
	{
	case WM_SIZE:
	{
		if (wParam == SIZE_RESTORED)
		{
			pWindow->ShowWindow();
		}
		else
		{
			pWindow->SetResized(true);
		}
		break;
	}

	default: break;
	}
}

Window::Window(const char* windowName, U32 width, U32 height, bool resizable)
	:m_windowName(windowName)
	,m_hWindow(NULL)
	,m_width(width)
	,m_height(height)
	,m_resizable(resizable)
	,m_isMinimized(false)
	,m_wasResized(false)
{
}

Window::~Window()
{
}

bool Window::Setup()
{
	WNDCLASSEX wc;
	MemZero(wc);
	wc.cbSize = sizeof(wc);
	wc.style = CS_OWNDC | CS_DBLCLKS;
	wc.lpfnWndProc = (WNDPROC)MainWindowMsgHandler;
	wc.hInstance = ::GetModuleHandle(NULL);
	wc.lpszClassName = WINDOW_CLASS_NAME;
	RegisterClassEx(&wc);

	DWORD windowStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE;

	if (m_resizable)
	{
		windowStyle |= WS_THICKFRAME;
	}

	// Have to calculate how big to create the window to ensure the "client area" (renderable surface) is actually sized to m_width and m_height
	RECT fullWindowSize;
	fullWindowSize.left = 0;
	fullWindowSize.top = 0;
	fullWindowSize.right = m_width;
	fullWindowSize.bottom = m_height;

	::AdjustWindowRectEx(&fullWindowSize, windowStyle, FALSE, NULL);
	I32 createWidth = fullWindowSize.right - fullWindowSize.left;
	I32 createHeight = fullWindowSize.bottom - fullWindowSize.top;

	m_hWindow = CreateWindowEx(0, WINDOW_CLASS_NAME, m_windowName, windowStyle, CW_USEDEFAULT, CW_USEDEFAULT, createWidth, createHeight, NULL, NULL, GetModuleHandle(NULL), this);
	ASSERT(NULL != m_hWindow);

	UpdateSize();

	AddMessageCallback(InternalWindowMessageHandler, this);

	// Make sure to show message on init
	ShowWindow();

	return true;
}

void Window::Update()
{
	m_wasResized = false;

	// Pump messages
	MSG msg;
	while (::PeekMessage(&msg, m_hWindow, 0, 0, PM_REMOVE))
	{
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
	}

	UpdateSize();
	m_isMinimized = ::IsIconic(m_hWindow);
}

void Window::UpdateSize()
{
	RECT windowSize;
	::GetClientRect(m_hWindow, &windowSize);
	m_width = windowSize.right - windowSize.left;
	m_height = windowSize.bottom - windowSize.top;
}

void Window::Teardown()
{
	::DestroyWindow(m_hWindow);
	::UnregisterClass(WINDOW_CLASS_NAME, ::GetModuleHandle(NULL));
}

HWND Window::GetHandle() const
{
	return m_hWindow;
}

void Window::SetTitle(const std::string& newTitle)
{
	LPCSTR newTitleWin32 = static_cast<LPCSTR>(newTitle.c_str());
	::SetWindowText(m_hWindow, newTitleWin32);
}

U32 Window::GetWidth() const
{
	return m_width;
}

U32 Window::GetHeight() const
{
	return m_height;
}

IntVec2 Window::GetSize() const
{
	return IntVec2(GetWidth(), GetHeight());
}

IntVec2 Window::GetPosition() const
{
	RECT screenRect;
	::GetWindowRect(m_hWindow, &screenRect);

	IntVec2 windowSize = GetSize();

	I32 screenPosX = screenRect.left + (windowSize.m_x / 2);
	I32 screenPosY = screenRect.top + (windowSize.m_y / 2);

	return IntVec2(screenPosX, screenPosY);
}

bool Window::IsMinimized() const
{
	return m_isMinimized;
}

void Window::SetResized(bool wasResized)
{
	m_wasResized = wasResized;
}

bool Window::WasResized() const
{
	return m_wasResized;
}

void Window::ShowWindow()
{
	IntVec2 pos = GetPosition();
	::ShowWindow(m_hWindow, SW_SHOW);
}

void Window::AddMessageCallback(WindowMessageCb msgCallback, void* userArgs)
{
	WindowMessageCbWithArgs cbWithArgs = { msgCallback, userArgs };
	m_messageCallbacks.push_back(cbWithArgs);
}

const std::vector<WindowMessageCbWithArgs>& Window::GetMessageCallbacks() const
{
	return m_messageCallbacks;
}
