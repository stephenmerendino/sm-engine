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
        Shader* CompileShader(ShaderType shaderType, 
                           const char* shaderFile, 
                           const char* entryFunctionName, 
                           LinearAllocator* allocator = GetCurrentAllocator());

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
        enum KeyCode : I32
        {
            kKeyInvalid = -1,
            kKey0,
            kKey1,
            kKey2,
            kKey3,
            kKey4,
            kKey5,
            kKey6,
            kKey7,
            kKey8,
            kKey9,

            kKeyA,
            kKeyB,
            kKeyC,
            kKeyD,
            kKeyE,
            kKeyF,
            kKeyG,
            kKeyH,
            kKeyI,
            kKeyJ,
            kKeyK,
            kKeyL,
            kKeyM,
            kKeyN,
            kKeyO,
            kKeyP,
            kKeyQ,
            kKeyR,
            kKeyS,
            kKeyT,
            kKeyU,
            kKeyV,
            kKeyW,
            kKeyX,
            kKeyY,
            kKeyZ,

            kMouseLButton,
            kMouseRButton,
            kMouseMButton,

            kKeyBackspace,
            kKeyTab,
            kKeyClear,
            kKeyEnter,
            kKeyShift,
            kKeyControl,
            kKeyAlt,
            kKeyPause,
            kKeyCapslock,
            kKeyEscape,
            kKeySpace,
            kKeyPageUp,
            kKeyPageDown,
            kKeyEnd,
            kKeyHome,
            kKeyLeftArrow,
            kKeyUpArrow,
            kKeyRightArrow,
            kKeyDownArrow,
            kKeySelect,
            kKeyPrint,
            kKeyPrintScreen,
            kKeyInsert,
            kKeyDelete,
            kKeyHelp,
            kKeySleep,

            kKeyNumPad0,
            kKeyNumPad1,
            kKeyNumPad2,
            kKeyNumPad3,
            kKeyNumPad4,
            kKeyNumPad5,
            kKeyNumPad6,
            kKeyNumPad7,
            kKeyNumPad8,
            kKeyNumPad9,
            kKeyMultiply,
            kKeyAdd,
            kKeySeparator,
            kKeySubtract,
            kKeyDecimal,
            kKeyDivide,

            kKeyF1,
            kKeyF2,
            kKeyF3,
            kKeyF4,
            kKeyF5,
            kKeyF6,
            kKeyF7,
            kKeyF8,
            kKeyF9,
            kKeyF10,
            kKeyF11,
            kKeyF12,
            kKeyF13,
            kKeyF14,
            kKeyF15,
            kKeyF16,
            kKeyF17,
            kKeyF18,
            kKeyF19,
            kKeyF20,
            kKeyF21,
            kKeyF22,
            kKeyF23,
            kKeyF24,
            kKeyNumLock,
            kKeyScrollLock,

            kNumKeyCodes
        };

        enum KeyStateBitFlags : U8
        {
            kIsDown         = 0x01,
            kWasPressed     = 0x02,
            kWasReleased    = 0x04 
        };

        bool IsKeyDown(KeyCode key);
        bool WasKeyPressed(KeyCode key);
        bool WasKeyReleased(KeyCode key);

        void ShowMouse();
        void HideMouse();
        bool IsMouseShown();
        void GetMousePositionScreen(U32& screenX, U32& screenY);
        void SetMousePositionScreen(U32 screenX, U32 screenY);
        void GetMousePositionNormalized(U32& screenX, U32& screenY);
        void SetMousePositionNormalized(U32 screenX, U32 screenY);
    } 
}
