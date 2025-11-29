#pragma once

#include "SM/Engine.h"

namespace SM
{ 
    namespace Platform
    {
        //------------------------------------------------------------------------------------------------------------------------
        // General
        void Exit();

        //------------------------------------------------------------------------------------------------------------------------
        // Logging
        void Log(const char* format, ...);

        //------------------------------------------------------------------------------------------------------------------------
        // Assertions
        bool AssertReportFailure(const char* expression, const char* filename, int lineNumber);
        bool AssertReportFailureMsg(const char* expression, const char* msg, const char* filename, int lineNumber);
        bool AssertReportError(const char* filename, int lineNumber);
        bool AssertReportErrorMsg(const char* msg, const char* filename, int lineNumber);
        void TriggerDebugger();
    } 
}
