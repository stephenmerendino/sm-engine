#pragma once

#include "Engine/Platform/WindowsInclude.h"
#include "Engine/Core/Types.h"
#include <vector>

typedef void (*WindowMsgCallbackFunc)(UINT msg, WPARAM wParam, LPARAM lParam, void* userArgs);
struct WindowMsgCallbackWithArgs 
{
	WindowMsgCallbackFunc m_cb;
	void* m_userArgs;
};

class Window
{
public:
	bool Init(const char* name, U32 width, U32 height, bool bResizable);
	//void Update();
	//void Destroy();

	//void SetTitle(const char* newTitle);
	//void AddMsgCallback(WindowMsgCallbackFunc cb, void* userArgs = nullptr);
	//IVec2 GetCenterPosition();

	HWND m_handle;
	const char* m_name;
	U32 m_width;
	U32 m_height;
	U32 m_x;
	U32 m_y;
	std::vector<WindowMsgCallbackWithArgs> m_msgCallbacks;
	bool m_bIsMinimized;
	bool m_bWasResized;
	bool m_bWasClosed;
	bool m_bIsMoving;
	Byte m_padding[4];
};
