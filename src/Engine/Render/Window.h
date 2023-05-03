#pragma once

#include "Engine/Core/Types.h"
#include "Engine/Math/IntVec2.h"
#include "Engine/Platform/WindowsInclude.h"

#include <vector>
#include <string>

typedef void (*WindowMessageCb)(UINT msg, WPARAM wParam, LPARAM lParam, void* userArgs);

struct WindowMessageCbWithArgs
{
	WindowMessageCb m_msgCb;
	void* m_userArgs;
};

class Window
{
public:
	Window(const char* windowName, U32 width, U32 height, bool resizable);
	~Window();

	bool Setup();
	void Update();
	void UpdateSize();
	void Teardown();

	HWND GetHandle() const;

	U32 GetWidth() const;
	U32 GetHeight() const;
	IntVec2 GetSize() const;
	IntVec2 GetPosition() const;
	bool IsMinimized() const;
	void SetResized(bool wasResized);
	bool WasResized() const;

	void SetTitle(const std::string& newTitle);
	void ShowWindow();

	void AddMessageCallback(WindowMessageCb msgCallback, void* userArgs);
	const std::vector<WindowMessageCbWithArgs>& GetMessageCallbacks() const;

private:
	const char* m_windowName;
	HWND m_hWindow;
	bool m_resizable;

	U32 m_width;
	U32 m_height;

	bool m_isMinimized;
	bool m_wasResized;

	std::vector<WindowMessageCbWithArgs> m_messageCallbacks;
};