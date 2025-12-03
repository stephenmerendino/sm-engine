#pragma once

#include "SM/Engine.h"
#include "SM/StandardTypes.h"

#define VK_NO_PROTOTYPES
#include "ThirdParty/vulkan/vulkan.h"

namespace SM
{ 
    namespace Platform
    {
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
    } 
}
