#pragma once

#include "SM/StandardTypes.h"

namespace SM
{ 
    namespace Platform
    {
        //------------------------------------------------------------------------------------------------------------------------
        // General
        //------------------------------------------------------------------------------------------------------------------------
        void Exit();

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
    } 
}
