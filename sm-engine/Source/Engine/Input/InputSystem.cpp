#include "Engine/Input/InputSystem.h"
#include "Engine/Core/BitFlags.h"
#include "Engine/Core/Macros.h"
#include "Engine/Render/Window.h"
#include "Engine/ThirdParty/imgui/imgui.h"

InputSystem g_inputSystem;

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
		case WM_KEYDOWN:	    g_inputSystem.HandleWinKeyDown((U32)wParam); break;
		case WM_SYSKEYDOWN:     g_inputSystem.HandleWinKeyDown((U32)wParam); break;
		case WM_LBUTTONDOWN:    g_inputSystem.HandleWinKeyDown(VK_LBUTTON); break;
		case WM_MBUTTONDOWN:    g_inputSystem.HandleWinKeyDown(VK_MBUTTON); break;
		case WM_RBUTTONDOWN:    g_inputSystem.HandleWinKeyDown(VK_RBUTTON); break;

		case WM_KEYUP:		    g_inputSystem.HandleWinKeyUp((U32)wParam); break;
		case WM_SYSKEYUP:	    g_inputSystem.HandleWinKeyUp((U32)wParam); break;
		case WM_LBUTTONUP:      g_inputSystem.HandleWinKeyUp(VK_LBUTTON); break;
		case WM_MBUTTONUP:      g_inputSystem.HandleWinKeyUp(VK_MBUTTON); break;
		case WM_RBUTTONUP:      g_inputSystem.HandleWinKeyUp(VK_RBUTTON); break;
	}
}

void KeyState::SetFlag(KeyStateBitFlags flag, bool flagValue)
{
	if (flagValue)
	{
		SetBit(m_state, (U8)flag);
	}
	else
	{
		UnsetBit(m_state, (U8)flag);
	}
}

bool KeyState::GetFlag(KeyStateBitFlags flag)
{
	return IsBitSet(m_state, (U8)flag);
}

I32 KeyState::GetIndexFromWin32Key(U32 windowsKey)
{
	// https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes

	// Numbers 0-9
	// 0x30 = 0, 0x39 = 9
	if (windowsKey >= 0x30 && windowsKey <= 0x39)
	{
		return (U32)KeyCode::KEY_0 + (windowsKey - 0x30);
	}

	// Letters A-Z
	// 0x41 = A, 0x5A = Z
	if (windowsKey >= 0x41 && windowsKey <= 0x5A)
	{
		return (U32)KeyCode::KEY_A + (windowsKey - 0x41);
	}

	// Numpad 0-9
	if (windowsKey >= VK_NUMPAD0 && windowsKey <= VK_NUMPAD9)
	{
		return (U32)KeyCode::KEY_NUMPAD0 + (windowsKey - VK_NUMPAD0);
	}

	// F1-F24
	if (windowsKey >= VK_F1 && windowsKey <= VK_F24)
	{
		return (U32)KeyCode::KEY_F1 + (windowsKey - VK_F1);
	}

	// Handle everything else directly
	switch (windowsKey)
	{
		case VK_LBUTTON:    return (U32)KeyCode::MOUSE_LBUTTON; 
		case VK_RBUTTON:    return (U32)KeyCode::MOUSE_RBUTTON; 
		case VK_MBUTTON:    return (U32)KeyCode::MOUSE_MBUTTON; 

		case VK_BACK:       return (U32)KeyCode::KEY_BACKSPACE; 
		case VK_TAB:        return (U32)KeyCode::KEY_TAB; 
		case VK_CLEAR:      return (U32)KeyCode::KEY_CLEAR; 
		case VK_RETURN:     return (U32)KeyCode::KEY_ENTER; 
		case VK_SHIFT:      return (U32)KeyCode::KEY_SHIFT; 
		case VK_CONTROL:    return (U32)KeyCode::KEY_CONTROL; 
		case VK_MENU:       return (U32)KeyCode::KEY_ALT; 
		case VK_PAUSE:      return (U32)KeyCode::KEY_PAUSE; 
		case VK_CAPITAL:    return (U32)KeyCode::KEY_CAPSLOCK; 
		case VK_ESCAPE:     return (U32)KeyCode::KEY_ESCAPE; 
		case VK_SPACE:      return (U32)KeyCode::KEY_SPACE; 
		case VK_PRIOR:      return (U32)KeyCode::KEY_PAGEUP; 
		case VK_NEXT:       return (U32)KeyCode::KEY_PAGEDOWN; 
		case VK_END:        return (U32)KeyCode::KEY_END; 
		case VK_HOME:       return (U32)KeyCode::KEY_HOME; 
		case VK_LEFT:       return (U32)KeyCode::KEY_LEFTARROW; 
		case VK_UP:         return (U32)KeyCode::KEY_UPARROW; 
		case VK_RIGHT:      return (U32)KeyCode::KEY_RIGHTARROW; 
		case VK_DOWN:       return (U32)KeyCode::KEY_DOWNARROW; 
		case VK_SELECT:     return (U32)KeyCode::KEY_SELECT; 
		case VK_PRINT:      return (U32)KeyCode::KEY_PRINT; 
		case VK_SNAPSHOT:   return (U32)KeyCode::KEY_PRINTSCREEN; 
		case VK_INSERT:     return (U32)KeyCode::KEY_INSERT; 
		case VK_DELETE:     return (U32)KeyCode::KEY_DELETE; 
		case VK_HELP:       return (U32)KeyCode::KEY_HELP; 
		case VK_SLEEP:      return (U32)KeyCode::KEY_SLEEP; 

		case VK_MULTIPLY:   return (U32)KeyCode::KEY_MULTIPLY; 
		case VK_ADD:        return (U32)KeyCode::KEY_ADD; 
		case VK_SEPARATOR:  return (U32)KeyCode::KEY_SEPARATOR; 
		case VK_SUBTRACT:   return (U32)KeyCode::KEY_SUBTRACT; 
		case VK_DECIMAL:    return (U32)KeyCode::KEY_DECIMAL; 
		case VK_DIVIDE:     return (U32)KeyCode::KEY_DIVIDE; 

		case VK_NUMLOCK:    return (U32)KeyCode::KEY_NUMLOCK; 
		case VK_SCROLL:     return (U32)KeyCode::KEY_SCROLLLOCK; 
	}

	return KeyState::kInvalidIndex;
}

void InputSystem::HandleWinKeyDown(U32 winKey)
{
	I32 keyStateIndex = KeyState::GetIndexFromWin32Key(winKey);
	if (keyStateIndex == KeyState::kInvalidIndex)
	{
		return;
	}

	KeyState& keyState = m_keyStates[keyStateIndex];
	if(!keyState.GetFlag(KeyStateBitFlags::IS_DOWN))
	{
		keyState.SetFlag(KeyStateBitFlags::WAS_PRESSED, true);
	}
	keyState.SetFlag(KeyStateBitFlags::IS_DOWN, true);
}

void InputSystem::HandleWinKeyUp(U32 winKey)
{
	I32 keyStateIndex = KeyState::GetIndexFromWin32Key(winKey);
	if (keyStateIndex == KeyState::kInvalidIndex)
	{
		return;
	}

	KeyState& keyState = m_keyStates[keyStateIndex];
	if(!keyState.GetFlag(KeyStateBitFlags::IS_DOWN))
	{
		keyState.SetFlag(KeyStateBitFlags::WAS_RELEASED, true);
	}
	keyState.SetFlag(KeyStateBitFlags::IS_DOWN, false);
}

void InputSystem::SaveMousePos()
{
	POINT mousePos;
	::GetCursorPos(&mousePos);
	m_savedMousePos = IVec2(mousePos.x, mousePos.y);
}

void InputSystem::RestoreMousePos()
{
	::SetCursorPos(m_savedMousePos.x, m_savedMousePos.y);
}

void InputSystem::CenterMouseOnScreen()
{
	IVec2 centerPos = m_pWindow->CalcCenterPosition();
	::SetCursorPos(centerPos.x, centerPos.y);
}

void InputSystem::UpdateMouseMovement()
{
	POINT mousePos;
	::GetCursorPos(&mousePos);

	IVec2 centerPos = m_pWindow->CalcCenterPosition();
	IVec2 windowSize = m_pWindow->GetSize();

	IVec2 delta = IVec2(mousePos.x, mousePos.y) - centerPos;

	m_mouseMovementNormalized = Vec2((F32)delta.x / (F32)windowSize.x, 
									 (F32)delta.y / (F32)windowSize.y);
}

void InputSystem::HideCursor()
{
	::ShowCursor(false);
	m_bMouseIsShown = false;
}

void InputSystem::ShowCursor()
{
	::ShowCursor(true);
	m_bMouseIsShown = true;
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

void InputSystem::Shutdown()
{
	m_pWindow->RemoveMsgCallback(InputSystemMsgHandler);
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
		m_keyStates[i].SetFlag(KeyStateBitFlags::WAS_PRESSED, false);
		m_keyStates[i].SetFlag(KeyStateBitFlags::WAS_RELEASED, false);
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
	return IsBitSet(m_keyStates[(U32)key].m_state, (U8)KeyStateBitFlags::IS_DOWN);
}

bool InputSystem::WasKeyPressed(KeyCode key) const
{
	return IsBitSet(m_keyStates[(U32)key].m_state, (U8)KeyStateBitFlags::WAS_PRESSED);
}

bool InputSystem::WasKeyReleased(KeyCode key) const
{
	return IsBitSet(m_keyStates[(U32)key].m_state, (U8)KeyStateBitFlags::WAS_RELEASED);
}

Vec2 InputSystem::GetMouseMovement() const
{
	return m_mouseMovementNormalized;
}
