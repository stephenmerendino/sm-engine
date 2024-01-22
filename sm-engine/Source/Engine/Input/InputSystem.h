#pragma once

#include "Engine/Core/Types.h"
#include "Engine/Math/IVec2.h"
#include "Engine/Math/Vec2.h"

enum class KeyCode : U32
{
	KEY_0,
	KEY_1,
	KEY_2,
	KEY_3,
	KEY_4,
	KEY_5,
	KEY_6,
	KEY_7,
	KEY_8,
	KEY_9,

	KEY_A,
	KEY_B,
	KEY_C,
	KEY_D,
	KEY_E,
	KEY_F,
	KEY_G,
	KEY_H,
	KEY_I,
	KEY_J,
	KEY_K,
	KEY_L,
	KEY_M,
	KEY_N,
	KEY_O,
	KEY_P,
	KEY_Q,
	KEY_R,
	KEY_S,
	KEY_T,
	KEY_U,
	KEY_V,
	KEY_W,
	KEY_X,
	KEY_Y,
	KEY_Z,

	MOUSE_LBUTTON,
	MOUSE_RBUTTON,
	MOUSE_MBUTTON,

	KEY_BACKSPACE,
	KEY_TAB,
	KEY_CLEAR,
	KEY_ENTER,
	KEY_SHIFT,
	KEY_CONTROL,
	KEY_ALT,
	KEY_PAUSE,
	KEY_CAPSLOCK,
	KEY_ESCAPE,
	KEY_SPACE,
	KEY_PAGEUP,
	KEY_PAGEDOWN,
	KEY_END,
	KEY_HOME,
	KEY_LEFTARROW,
	KEY_UPARROW,
	KEY_RIGHTARROW,
	KEY_DOWNARROW,
	KEY_SELECT,
	KEY_PRINT,
	KEY_PRINTSCREEN,
	KEY_INSERT,
	KEY_DELETE,
	KEY_HELP,
	KEY_SLEEP,

	KEY_NUMPAD0,
	KEY_NUMPAD1,
	KEY_NUMPAD2,
	KEY_NUMPAD3,
	KEY_NUMPAD4,
	KEY_NUMPAD5,
	KEY_NUMPAD6,
	KEY_NUMPAD7,
	KEY_NUMPAD8,
	KEY_NUMPAD9,
	KEY_MULTIPLY,
	KEY_ADD,
	KEY_SEPARATOR,
	KEY_SUBTRACT,
	KEY_DECIMAL,
	KEY_DIVIDE,

	KEY_F1,
	KEY_F2,
	KEY_F3,
	KEY_F4,
	KEY_F5,
	KEY_F6,
	KEY_F7,
	KEY_F8,
	KEY_F9,
	KEY_F10,
	KEY_F11,
	KEY_F12,
	KEY_F13,
	KEY_F14,
	KEY_F15,
	KEY_F16,
	KEY_F17,
	KEY_F18,
	KEY_F19,
	KEY_F20,
	KEY_F21,
	KEY_F22,
	KEY_F23,
	KEY_F24,
	KEY_NUMLOCK,
	KEY_SCROLLLOCK,

	KEY_INVALID,

	NUM_KEY_CODES
};

enum class KeyStateBitFlags : U8
{
	IS_DOWN = 0b000000001,
	WAS_PRESSED = 0b000000010,
	WAS_RELEASED = 0b000000100
};

struct KeyState
{
	void SetFlag(KeyStateBitFlags flag, bool value);
	bool GetFlag(KeyStateBitFlags flag);

	static I32 GetIndexFromWin32Key(U32 windowsKey);

	U8 m_state = 0;

	enum
	{
		kInvalidIndex = -1
	};
};

class Window;
class InputSystem
{
public:
	InputSystem();

	void Init(Window* pWindow);
	void Shutdown();

	void BeginFrame();
	void Update();
	bool IsKeyDown(KeyCode key) const;
	bool WasKeyPressed(KeyCode key) const;
	bool WasKeyReleased(KeyCode key) const;
	void HandleWinKeyDown(U32 winKey);
	void HandleWinKeyUp(U32 winKey);
	Vec2 GetMouseMovement() const;
	void ResetAllInputState();

	void SaveMousePos();
	void RestoreMousePos();
	void CenterMouseOnScreen();
	void UpdateMouseMovement();
	void HideCursor();
	void ShowCursor();

	KeyState m_keyStates[(U32)KeyCode::NUM_KEY_CODES];
	Vec2 m_mouseMovementNormalized;
	bool m_bMouseIsShown;
	bool m_bUnhideMouse;
	bool m_bHideMouse;
	IVec2 m_savedMousePos;
	Window* m_pWindow;
};

extern InputSystem g_inputSystem;
