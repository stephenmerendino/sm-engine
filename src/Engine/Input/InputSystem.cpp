#include "Engine/Input/InputSystem.h"

#include "Engine/Core/Debug.h"
#include "Engine/Core/Common.h"
#include "Engine/Render/Window.h"

InputSystem g_inputSystem;

void WindowMessageHandler(UINT msg, WPARAM wParam, LPARAM lParam, void* userArgs)
{
	Unused(lParam);
	InputSystem* pInputSystem = (InputSystem*)userArgs;

	switch (msg)
	{
		// Keyboard / mouse
		case WM_KEYDOWN:	pInputSystem->HandleWindowsKeyDown(static_cast<U32>(wParam)); break;
		case WM_SYSKEYDOWN: pInputSystem->HandleWindowsKeyDown(static_cast<U32>(wParam)); break;
		case WM_LBUTTONDOWN: pInputSystem->HandleWindowsKeyDown(static_cast<U32>(wParam)); break;
		case WM_MBUTTONDOWN: pInputSystem->HandleWindowsKeyDown(static_cast<U32>(wParam)); break;
		case WM_RBUTTONDOWN: pInputSystem->HandleWindowsKeyDown(static_cast<U32>(wParam)); break;
		
		case WM_KEYUP:		pInputSystem->HandleWindowsKeyUp(static_cast<U32>(wParam)); break;
		case WM_SYSKEYUP:	pInputSystem->HandleWindowsKeyUp(static_cast<U32>(wParam)); break;
		case WM_LBUTTONUP: pInputSystem->HandleMouseButtonUp(KeyCode::MOUSE_LBUTTON); break;
		case WM_MBUTTONUP: pInputSystem->HandleMouseButtonUp(KeyCode::MOUSE_MBUTTON); break;
		case WM_RBUTTONUP: pInputSystem->HandleMouseButtonUp(KeyCode::MOUSE_RBUTTON); break;

		// Mouse double clicks (if needed in future)
		case WM_LBUTTONDBLCLK: break;
		case WM_MBUTTONDBLCLK: break;
		case WM_RBUTTONDBLCLK: break;
	}
}

KeyState::KeyState()
	:m_state(0)
{
}

void KeyState::Reset()
{
	m_state = 0;
}

bool KeyState::GetIsDown() const
{
	return IsBitSet(m_state, (U8)KeyStateBitFlags::IS_DOWN);
}

bool KeyState::GetWasPressed() const
{
	return IsBitSet(m_state, (U8)KeyStateBitFlags::WAS_PRESSED);
}

bool KeyState::GetWasReleased() const
{
	return IsBitSet(m_state, (U8)KeyStateBitFlags::WAS_RELEASED);
}

void KeyState::SetIsDown(bool isDown)
{
	if (isDown)
	{
		SetBit(m_state, (U8)KeyStateBitFlags::IS_DOWN);
	}
	else
	{
		UnsetBit(m_state, (U8)KeyStateBitFlags::IS_DOWN);
	}
}

void KeyState::SetWasPressed(bool wasPressed)
{
	if (wasPressed)
	{
		SetBit(m_state, (U8)KeyStateBitFlags::WAS_PRESSED);
	}
	else
	{
		UnsetBit(m_state, (U8)KeyStateBitFlags::WAS_PRESSED);
	}
}

void KeyState::SetWasReleased(bool wasReleased)
{
	if (wasReleased)
	{
		SetBit(m_state, (U8)KeyStateBitFlags::WAS_RELEASED);
	}
	else
	{
		UnsetBit(m_state, (U8)KeyStateBitFlags::WAS_RELEASED);
	}
}

InputSystem::InputSystem()
	:m_pWindow(nullptr)
	,m_mouseIsShown(true)
	,m_mouseMovementNormalized(Vec2::ZERO)
	,m_savedMousePosition(IntVec2::ZERO)
{
}

InputSystem::~InputSystem()
{
}

bool InputSystem::Init(Window* pWindow)
{
	m_pWindow = pWindow;
	m_pWindow->AddMessageCallback(WindowMessageHandler, this);
	return true;
}

void InputSystem::Shutdown()
{
}

void InputSystem::BeforeWindowUpdate()
{
	for (int i = 0; i < (U32)KeyCode::NUM_KEY_CODES; i++)
	{
		m_keyStates[i].SetWasPressed(false);
		m_keyStates[i].SetWasReleased(false);
	}
}

void InputSystem::Update()
{
	// Mouse movement update
	{
		// Always reset mouse movement to zero
		m_mouseMovementNormalized = Vec2::ZERO;

		if (WasKeyReleased(KeyCode::MOUSE_RBUTTON) && !m_mouseIsShown)
		{
			ShowCursor();
			RestoreMousePosition();
		}
		else if (WasKeyPressed(KeyCode::MOUSE_RBUTTON) && m_mouseIsShown)
		{
			HideCursor();
			SaveMousePosition();
			CenterMouseOnScreen();
		}
		else if (IsKeyDown(KeyCode::MOUSE_RBUTTON))
		{
			UpdateMouseMovement();
			CenterMouseOnScreen();
		}
	}
}

void InputSystem::SaveMousePosition()
{
	POINT mousePos;
	::GetCursorPos(&mousePos);

	m_savedMousePosition = IntVec2(mousePos.x, mousePos.y);
}

void InputSystem::RestoreMousePosition()
{
	::SetCursorPos(m_savedMousePosition.m_x, m_savedMousePosition.m_y);
}

void InputSystem::CenterMouseOnScreen()
{
	IntVec2 windowPos = m_pWindow->GetPosition();
	::SetCursorPos(windowPos.m_x, windowPos.m_y);
}

void InputSystem::UpdateMouseMovement()
{
	POINT mousePos;
	::GetCursorPos(&mousePos);

	IntVec2 windowPos = m_pWindow->GetPosition();
	IntVec2 windowSize = m_pWindow->GetSize();

	IntVec2 deltaMovement = IntVec2(mousePos.x, mousePos.y) - windowPos;

	m_mouseMovementNormalized = Vec2((F32)deltaMovement.m_x / (F32)windowSize.m_x,
									 (F32)deltaMovement.m_y / (F32)windowSize.m_y);
}

void InputSystem::HideCursor()
{
	::ShowCursor(false);
	m_mouseIsShown = false;
}

void InputSystem::ShowCursor()
{
	::ShowCursor(true);
	m_mouseIsShown = true;
}

bool InputSystem::IsKeyDown(KeyCode key) const
{
	return m_keyStates[(U32)key].GetIsDown();
}

bool InputSystem::WasKeyPressed(KeyCode key) const
{
	return m_keyStates[(U32)key].GetWasPressed();
}

bool InputSystem::WasKeyReleased(KeyCode key) const
{
	return m_keyStates[(U32)key].GetWasReleased();
}

Vec2 InputSystem::GetMouseMovement() const
{
	return m_mouseMovementNormalized;
}

bool InputSystem::GetKeyStateIndexForWindowsKey(int* outKeyStateIndex, U32 winKeyCode) const
{
	// https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
	
	// Numbers 0-9
	// 0x30 = 0, 0x39 = 9
	if (winKeyCode >= 0x30 && winKeyCode <= 0x39)
	{
		*outKeyStateIndex = (U32)KeyCode::KEY_0 + (winKeyCode - 0x30);
		return true;
	}

	// Letters A-Z
	// 0x41 = A, 0x5A = Z
	if (winKeyCode >= 0x41 && winKeyCode <= 0x5A)
	{
		*outKeyStateIndex = (U32)KeyCode::KEY_A + (winKeyCode - 0x41);
		return true;
	}

	// Numpad 0-9
	if (winKeyCode >= VK_NUMPAD0 && winKeyCode <= VK_NUMPAD9)
	{
		*outKeyStateIndex = (U32)KeyCode::KEY_NUMPAD0 + (winKeyCode - VK_NUMPAD0);
		return true;
	}

	// F1-F24
	if (winKeyCode >= VK_F1 && winKeyCode <= VK_F24)
	{
		*outKeyStateIndex = (U32)KeyCode::KEY_F1 + (winKeyCode - VK_F1);
		return true;
	}

	// Handle everything else directly
	switch (winKeyCode)
	{
		case VK_LBUTTON: *outKeyStateIndex = (U32)KeyCode::MOUSE_LBUTTON; return true;
		case VK_RBUTTON: *outKeyStateIndex = (U32)KeyCode::MOUSE_RBUTTON; return true;
		case VK_MBUTTON: *outKeyStateIndex = (U32)KeyCode::MOUSE_MBUTTON; return true;

		case VK_BACK: *outKeyStateIndex = (U32)KeyCode::KEY_BACKSPACE; return true;
		case VK_TAB: *outKeyStateIndex = (U32)KeyCode::KEY_TAB; return true;
		case VK_CLEAR: *outKeyStateIndex = (U32)KeyCode::KEY_CLEAR; return true;
		case VK_RETURN: *outKeyStateIndex = (U32)KeyCode::KEY_ENTER; return true;
		case VK_SHIFT: *outKeyStateIndex = (U32)KeyCode::KEY_SHIFT; return true;
		case VK_CONTROL: *outKeyStateIndex = (U32)KeyCode::KEY_CONTROL; return true;
		case VK_MENU: *outKeyStateIndex = (U32)KeyCode::KEY_ALT; return true;
		case VK_PAUSE: *outKeyStateIndex = (U32)KeyCode::KEY_PAUSE; return true;
		case VK_CAPITAL: *outKeyStateIndex = (U32)KeyCode::KEY_CAPSLOCK; return true;
		case VK_ESCAPE: *outKeyStateIndex = (U32)KeyCode::KEY_ESCAPE; return true;
		case VK_SPACE: *outKeyStateIndex = (U32)KeyCode::KEY_SPACE; return true;
		case VK_PRIOR: *outKeyStateIndex = (U32)KeyCode::KEY_PAGEUP; return true;
		case VK_NEXT: *outKeyStateIndex = (U32)KeyCode::KEY_PAGEDOWN; return true;
		case VK_END: *outKeyStateIndex = (U32)KeyCode::KEY_END; return true;
		case VK_HOME: *outKeyStateIndex = (U32)KeyCode::KEY_HOME; return true;
		case VK_LEFT: *outKeyStateIndex = (U32)KeyCode::KEY_LEFTARROW; return true;
		case VK_UP: *outKeyStateIndex = (U32)KeyCode::KEY_UPARROW; return true;
		case VK_RIGHT: *outKeyStateIndex = (U32)KeyCode::KEY_RIGHTARROW; return true;
		case VK_DOWN: *outKeyStateIndex = (U32)KeyCode::KEY_DOWNARROW; return true;
		case VK_SELECT: *outKeyStateIndex = (U32)KeyCode::KEY_SELECT; return true;
		case VK_PRINT: *outKeyStateIndex = (U32)KeyCode::KEY_PRINT; return true;
		case VK_SNAPSHOT: *outKeyStateIndex = (U32)KeyCode::KEY_PRINTSCREEN; return true;
		case VK_INSERT: *outKeyStateIndex = (U32)KeyCode::KEY_INSERT; return true;
		case VK_DELETE: *outKeyStateIndex = (U32)KeyCode::KEY_DELETE; return true;
		case VK_HELP: *outKeyStateIndex = (U32)KeyCode::KEY_HELP; return true;
		case VK_SLEEP: *outKeyStateIndex = (U32)KeyCode::KEY_SLEEP; return true;

		case VK_MULTIPLY: *outKeyStateIndex = (U32)KeyCode::KEY_MULTIPLY; return true;
		case VK_ADD: *outKeyStateIndex = (U32)KeyCode::KEY_ADD; return true;
		case VK_SEPARATOR: *outKeyStateIndex = (U32)KeyCode::KEY_SEPARATOR; return true;
		case VK_SUBTRACT: *outKeyStateIndex = (U32)KeyCode::KEY_SUBTRACT; return true;
		case VK_DECIMAL: *outKeyStateIndex = (U32)KeyCode::KEY_DECIMAL; return true;
		case VK_DIVIDE: *outKeyStateIndex = (U32)KeyCode::KEY_DIVIDE; return true;

		case VK_NUMLOCK: *outKeyStateIndex = (U32)KeyCode::KEY_NUMLOCK; return true;
		case VK_SCROLL: *outKeyStateIndex = (U32)KeyCode::KEY_SCROLLLOCK; return true;
	}

	return false;
}

void InputSystem::HandleWindowsKeyDown(U32 winKeyCode)
{
	int keyStateIndex = 0;
	if (!GetKeyStateIndexForWindowsKey(&keyStateIndex, winKeyCode))
	{
		return;
	}

	KeyState& keyState = m_keyStates[keyStateIndex];
	if (!keyState.GetIsDown())
	{
		keyState.SetWasPressed(true);
	}
	keyState.SetIsDown(true);
}

void InputSystem::HandleWindowsKeyUp(U32 winKeyCode)
{
	int keyStateIndex = -1;
	if (!GetKeyStateIndexForWindowsKey(&keyStateIndex, winKeyCode))
	{
		return;
	}

	KeyState& keyState = m_keyStates[keyStateIndex];
	if (keyState.GetIsDown())
	{
		keyState.SetWasReleased(true);
	}
	keyState.SetIsDown(false);
}

void InputSystem::HandleMouseButtonUp(KeyCode mouseButton)
{
	KeyState& keyState = m_keyStates[(U32)mouseButton];
	if (keyState.GetIsDown())
	{
		keyState.SetWasReleased(true);
	}
	keyState.SetIsDown(false);
}