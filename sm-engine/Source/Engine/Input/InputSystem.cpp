#include "Engine/Input/InputSystem.h"
#include "Engine/Core/BitFlags.h"
#include "Engine/Core/Macros.h"
#include "Engine/Render/Window.h"
#include "Engine/ThirdParty/imgui/imgui.h"

//#include "engine/core/debug.h"
//#include <vcruntime_string.h>
//#include <winuser.h>

InputSystem g_inputSystem;

enum class KeyStateBitFlags : U8
{
	IS_DOWN = 0b000000001,
	WAS_PRESSED = 0b000000010,
	WAS_RELEASED = 0b000000100
};

static void SetKeyStateFlag(KeyState& keyState, KeyStateBitFlags flag, bool flagValue)
{
	if (flagValue)
	{
		SetBit(keyState.state, (U8)flag);
	}
	else
	{
		UnsetBit(keyState.state, (U8)flag);
	}
}

static bool GetKeyStateFlag(const KeyState& keyState, KeyStateBitFlags flag)
{
	return IsBitSet(keyState.state, (U8)flag);
}

static bool GetKeyStateIndexFromWinKey(int* outIndex, U32 winKey)
{
	// https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes

	// Numbers 0-9
	// 0x30 = 0, 0x39 = 9
	if (winKey >= 0x30 && winKey <= 0x39)
	{
		*outIndex = (U32)KeyCode::KEY_0 + (winKey - 0x30);
		return true;
	}

	// Letters A-Z
	// 0x41 = A, 0x5A = Z
	if (winKey >= 0x41 && winKey <= 0x5A)
	{
		*outIndex = (U32)KeyCode::KEY_A + (winKey - 0x41);
		return true;
	}

	// Numpad 0-9
	if (winKey >= VK_NUMPAD0 && winKey <= VK_NUMPAD9)
	{
		*outIndex = (U32)KeyCode::KEY_NUMPAD0 + (winKey - VK_NUMPAD0);
		return true;
	}

	// F1-F24
	if (winKey >= VK_F1 && winKey <= VK_F24)
	{
		*outIndex = (U32)KeyCode::KEY_F1 + (winKey - VK_F1);
		return true;
	}

	// Handle everything else directly
	switch (winKey)
	{
		case VK_LBUTTON:    *outIndex = (U32)KeyCode::MOUSE_LBUTTON; return true;
		case VK_RBUTTON:    *outIndex = (U32)KeyCode::MOUSE_RBUTTON; return true;
		case VK_MBUTTON:    *outIndex = (U32)KeyCode::MOUSE_MBUTTON; return true;

		case VK_BACK:       *outIndex = (U32)KeyCode::KEY_BACKSPACE; return true;
		case VK_TAB:        *outIndex = (U32)KeyCode::KEY_TAB; return true;
		case VK_CLEAR:      *outIndex = (U32)KeyCode::KEY_CLEAR; return true;
		case VK_RETURN:     *outIndex = (U32)KeyCode::KEY_ENTER; return true;
		case VK_SHIFT:      *outIndex = (U32)KeyCode::KEY_SHIFT; return true;
		case VK_CONTROL:    *outIndex = (U32)KeyCode::KEY_CONTROL; return true;
		case VK_MENU:       *outIndex = (U32)KeyCode::KEY_ALT; return true;
		case VK_PAUSE:      *outIndex = (U32)KeyCode::KEY_PAUSE; return true;
		case VK_CAPITAL:    *outIndex = (U32)KeyCode::KEY_CAPSLOCK; return true;
		case VK_ESCAPE:     *outIndex = (U32)KeyCode::KEY_ESCAPE; return true;
		case VK_SPACE:      *outIndex = (U32)KeyCode::KEY_SPACE; return true;
		case VK_PRIOR:      *outIndex = (U32)KeyCode::KEY_PAGEUP; return true;
		case VK_NEXT:       *outIndex = (U32)KeyCode::KEY_PAGEDOWN; return true;
		case VK_END:        *outIndex = (U32)KeyCode::KEY_END; return true;
		case VK_HOME:       *outIndex = (U32)KeyCode::KEY_HOME; return true;
		case VK_LEFT:       *outIndex = (U32)KeyCode::KEY_LEFTARROW; return true;
		case VK_UP:         *outIndex = (U32)KeyCode::KEY_UPARROW; return true;
		case VK_RIGHT:      *outIndex = (U32)KeyCode::KEY_RIGHTARROW; return true;
		case VK_DOWN:       *outIndex = (U32)KeyCode::KEY_DOWNARROW; return true;
		case VK_SELECT:     *outIndex = (U32)KeyCode::KEY_SELECT; return true;
		case VK_PRINT:      *outIndex = (U32)KeyCode::KEY_PRINT; return true;
		case VK_SNAPSHOT:   *outIndex = (U32)KeyCode::KEY_PRINTSCREEN; return true;
		case VK_INSERT:     *outIndex = (U32)KeyCode::KEY_INSERT; return true;
		case VK_DELETE:     *outIndex = (U32)KeyCode::KEY_DELETE; return true;
		case VK_HELP:       *outIndex = (U32)KeyCode::KEY_HELP; return true;
		case VK_SLEEP:      *outIndex = (U32)KeyCode::KEY_SLEEP; return true;

		case VK_MULTIPLY:   *outIndex = (U32)KeyCode::KEY_MULTIPLY; return true;
		case VK_ADD:        *outIndex = (U32)KeyCode::KEY_ADD; return true;
		case VK_SEPARATOR:  *outIndex = (U32)KeyCode::KEY_SEPARATOR; return true;
		case VK_SUBTRACT:   *outIndex = (U32)KeyCode::KEY_SUBTRACT; return true;
		case VK_DECIMAL:    *outIndex = (U32)KeyCode::KEY_DECIMAL; return true;
		case VK_DIVIDE:     *outIndex = (U32)KeyCode::KEY_DIVIDE; return true;

		case VK_NUMLOCK:    *outIndex = (U32)KeyCode::KEY_NUMLOCK; return true;
		case VK_SCROLL:     *outIndex = (U32)KeyCode::KEY_SCROLLLOCK; return true;
	}

	return false;
}

static void HandleWinKeyDown(U32 winKey)
{
	int keyStateIndex = -1;
	if (!GetKeyStateIndexFromWinKey(&keyStateIndex, winKey))
	{
		return;
	}

	KeyState& keyState = g_inputSystem.m_keyStates[keyStateIndex];
	if (!GetKeyStateFlag(keyState, KeyStateBitFlags::IS_DOWN))
	{
		SetKeyStateFlag(keyState, KeyStateBitFlags::WAS_PRESSED, true);
	}
	SetKeyStateFlag(keyState, KeyStateBitFlags::IS_DOWN, true);
}

static void HandleWinKeyUp(U32 winKey)
{
	int keyStateIndex = -1;
	if (!GetKeyStateIndexFromWinKey(&keyStateIndex, winKey))
	{
		return;
	}

	KeyState& keyState = g_inputSystem.m_keyStates[keyStateIndex];
	if (!GetKeyStateFlag(keyState, KeyStateBitFlags::IS_DOWN))
	{
		SetKeyStateFlag(keyState, KeyStateBitFlags::WAS_RELEASED, true);
	}
	SetKeyStateFlag(keyState, KeyStateBitFlags::IS_DOWN, false);
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
static bool ImGuiMsgHandler(UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (!ImGui::GetCurrentContext()) return false;

	ImGuiIO& io = ImGui::GetIO();
	bool mouse_handled_by_imgui = io.WantSetMousePos || io.WantCaptureMouse;
	if (!mouse_handled_by_imgui)
	{
		// flag to unhide the mouse in input_update
		if (msg == WM_RBUTTONUP && !g_inputSystem.m_bMouseIsShown)
		{
			g_inputSystem.m_bUnhideMouse = true;
		}

		// flag to hide the mouse in input_update
		if (msg == WM_RBUTTONDOWN && g_inputSystem.m_bMouseIsShown)
		{
			g_inputSystem.m_bHideMouse = true;
		}
	}

	bool input_handled_by_imgui = ImGui_ImplWin32_WndProcHandler(g_inputSystem.m_pWindow->m_handle, msg, wParam, lParam);
	return input_handled_by_imgui || mouse_handled_by_imgui;
}

static void InputSystemMsgHandler(UINT msg, WPARAM wParam, LPARAM lParam, void* userArgs)
{
	UNUSED(lParam);
	UNUSED(userArgs);

	if (ImGuiMsgHandler(msg, wParam, lParam))
	{
		g_inputSystem.ResetAllInputState();
		return;
	}

	switch (msg)
	{
		// Keyboard / mouse
		case WM_KEYDOWN:	    HandleWinKeyDown((U32)wParam); break;
		case WM_SYSKEYDOWN:     HandleWinKeyDown((U32)wParam); break;
		case WM_LBUTTONDOWN:    HandleWinKeyDown(VK_LBUTTON); break;
		case WM_MBUTTONDOWN:    HandleWinKeyDown(VK_MBUTTON); break;
		case WM_RBUTTONDOWN:    HandleWinKeyDown(VK_RBUTTON); break;

		case WM_KEYUP:		    HandleWinKeyUp((U32)wParam); break;
		case WM_SYSKEYUP:	    HandleWinKeyUp((U32)wParam); break;
		case WM_LBUTTONUP:      HandleWinKeyUp(VK_LBUTTON); break;
		case WM_MBUTTONUP:      HandleWinKeyUp(VK_MBUTTON); break;
		case WM_RBUTTONUP:      HandleWinKeyUp(VK_RBUTTON); break;
	}
}

static void SaveMousePos()
{
	POINT mousePos;
	::GetCursorPos(&mousePos);
	g_inputSystem.m_savedMousePos = IVec2(mousePos.x, mousePos.y);
}

static void RestoreMousePos()
{
	::SetCursorPos(g_inputSystem.m_savedMousePos.x, g_inputSystem.m_savedMousePos.y);
}

static void CenterMouseOnScreen()
{
	IVec2 centerPos = g_inputSystem.m_pWindow->CalcCenterPosition();
	::SetCursorPos(centerPos.x, centerPos.y);
}

static void UpdateMouseMovement()
{
	POINT mousePos;
	::GetCursorPos(&mousePos);

	IVec2 centerPos = g_inputSystem.m_pWindow->CalcCenterPosition();
	IVec2 windowSize = g_inputSystem.m_pWindow->GetSize();

	IVec2 delta = IVec2(mousePos.x, mousePos.y) - centerPos;

	g_inputSystem.m_mouseMovementNormalized = Vec2((F32)delta.x / (F32)windowSize.x,
												   (F32)delta.y / (F32)windowSize.y);
}

static void HideCursor()
{
	::ShowCursor(false);
	g_inputSystem.m_bMouseIsShown = false;
}

static void ShowCursor()
{
	::ShowCursor(true);
	g_inputSystem.m_bMouseIsShown = true;
}

InputSystem::InputSystem()
	:m_mouseMovementNormalized(Vec2::ZERO)
	,m_bMouseIsShown(true)
	,m_bUnhideMouse(false)
	,m_bHideMouse(false)
	,m_savedMousePos(IVec2::ZERO)
	,m_pWindow(nullptr)
{
}

bool InputSystem::Init(Window* pWindow)
{
	SM_ASSERT(nullptr != pWindow);
	m_pWindow = pWindow;
	m_pWindow->AddMsgCallback(InputSystemMsgHandler);
	return true;
}

void InputSystem::ResetAllInputState()
{
	memset(m_keyStates, 0, (U32)KeyCode::NUM_KEY_CODES * sizeof(KeyState));
}

void InputSystem::BeginFrame()
{
	m_mouseMovementNormalized = Vec2::ZERO;
	for (int i = 0; i < (U32)KeyCode::NUM_KEY_CODES; i++)
	{
		SetKeyStateFlag(m_keyStates[i], KeyStateBitFlags::WAS_PRESSED, false);
		SetKeyStateFlag(m_keyStates[i], KeyStateBitFlags::WAS_RELEASED, false);
	}
}

void InputSystem::Update()
{
	// Mouse movement update
	if (m_bUnhideMouse)
	{
		m_bUnhideMouse = false;
		ShowCursor();
		RestoreMousePos();
	}
	else if (m_bHideMouse)
	{
		m_bHideMouse = false;
		HideCursor();
		SaveMousePos();
		CenterMouseOnScreen();
	}
	else if (IsKeyDown(KeyCode::MOUSE_RBUTTON))
	{
		UpdateMouseMovement();
		CenterMouseOnScreen();
	}
}

bool InputSystem::IsKeyDown(KeyCode key) const
{
	return IsBitSet(m_keyStates[(U32)key].state, (U8)KeyStateBitFlags::IS_DOWN);
}

bool InputSystem::WasKeyPressed(KeyCode key) const
{
	return IsBitSet(m_keyStates[(U32)key].state, (U8)KeyStateBitFlags::WAS_PRESSED);
}

bool InputSystem::WasKeyReleased(KeyCode key) const
{
	return IsBitSet(m_keyStates[(U32)key].state, (U8)KeyStateBitFlags::WAS_RELEASED);
}

Vec2 InputSystem::GetMouseMovement() const
{
	return m_mouseMovementNormalized;
}
