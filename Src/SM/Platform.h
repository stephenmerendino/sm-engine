#pragma once

#include "SM/Engine.h"
#include "SM/Memory.h"
#include "SM/StandardTypes.h"
#include "SM/Renderer/VulkanRenderer.h"

#define VK_NO_PROTOTYPES
#include "ThirdParty/vulkan/vulkan.h"

namespace SM
{ 
    namespace Platform
    {
        //------------------------------------------------------------------------------------------------------------------------
        // General
        //------------------------------------------------------------------------------------------------------------------------
        void Init();

        //------------------------------------------------------------------------------------------------------------------------
        // Game
        //------------------------------------------------------------------------------------------------------------------------
        GameApi LoadGameDll(const char* gameDll);

        //------------------------------------------------------------------------------------------------------------------------
        // Logging
        //------------------------------------------------------------------------------------------------------------------------
        void Log(const char* format, ...);

        //------------------------------------------------------------------------------------------------------------------------
        // Assertions
        //------------------------------------------------------------------------------------------------------------------------
        bool AssertReportFailure(const char* expression, const char* filename, I32 lineNumber);
        bool AssertReportFailureMsg(const char* expression, const char* msg, const char* filename, I32 lineNumber);
        bool AssertReportError(const char* filename, I32 lineNumber);
        bool AssertReportErrorMsg(const char* msg, const char* filename, I32 lineNumber);
        void TriggerDebugger();

        //------------------------------------------------------------------------------------------------------------------------
        // Window
        //------------------------------------------------------------------------------------------------------------------------
        struct Window;
        Window* OpenWindow(const char* title, U32 width, U32 height);
        void UpdateWindow(Window* pWindow);
        void GetWindowDimensions(Window* pWindow, U32& width, U32& height);

        //------------------------------------------------------------------------------------------------------------------------
        // Rendering
        //------------------------------------------------------------------------------------------------------------------------
        void LoadVulkanGlobalFuncs();
        void LoadVulkanInstanceFuncs(VkInstance instance);
        void LoadVulkanDeviceFuncs(VkDevice device);
        PFN_vkDebugUtilsMessengerCallbackEXT GetVulkanDebugCallback();
        VkSurfaceKHR CreateVulkanSurface(VkInstance instance, Window* platformWindow);
        void CompileShader(ShaderType shaderType, const char* shaderFile, const char* entryFunctionName);

        //------------------------------------------------------------------------------------------------------------------------
        // Timing
        //------------------------------------------------------------------------------------------------------------------------
        F32 GetMillisecondsSinceAppStart();
        F32 GetSecondsSinceAppStart();

        //------------------------------------------------------------------------------------------------------------------------
        // Threads
        //------------------------------------------------------------------------------------------------------------------------
        void YieldThread();
        void SleepThreadSeconds(F32 seconds);
        void SleepThreadMilliseconds(F32 ms);

        //------------------------------------------------------------------------------------------------------------------------
        // File I/O
        //------------------------------------------------------------------------------------------------------------------------
        bool ReadFileBytes(const char* filename, Byte*& outBytes, size_t& outNumBytes, LinearAllocator* allocator = GetCurrentAllocator());

        //------------------------------------------------------------------------------------------------------------------------
        // Keyboard / Mouse / Gamepad Input
        //------------------------------------------------------------------------------------------------------------------------
        enum KeyCode : U32
        {
            kKey0,
            kKey1,
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

        enum KeyStateBitFlags : U8
        {
            kIsDown         = 0x01,
            kWasPressed     = 0x02,
            kWasReleased    = 0x04 
        };
    } 
}
