#pragma once

#include "SM/Engine.h"

namespace sm 
{ 
    namespace Platform
    {
        //------------------------------------------------------------------------------------------------------------------------
        void Log(const char* format, ...);

        //------------------------------------------------------------------------------------------------------------------------
        // Assertions
        bool AssertReportFailure(const char* expression, const char* filename, int lineNumber);
        bool AssertReportFailureMsg(const char* expression, const char* msg, const char* filename, int lineNumber);
        bool AssertReportError(const char* filename, int lineNumber);
        bool AssertReportErrorMsg(const char* msg, const char* filename, int lineNumber);
        void TriggerDebugger();

        //------------------------------------------------------------------------------------------------------------------------
        class Window
        {
            public:
                virtual int GetWidth() = 0;
                virtual int GetHeight() = 0;
        };
        Window* InitWindow(I32 width, I32 height);
    } 
}
