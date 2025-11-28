#pragma once

#include "SM/Platform.h"

namespace sm
{
    #define MAX_ASSERT_MSG_LEN 2048

    #define ASSERTIONS_ENABLED !NDEBUG

    #if ASSERTIONS_ENABLED
        #define SM_ASSERT(expr) \
            if(expr){} \
            else if(Platform::AssertReportFailure(#expr, __FILE__, __LINE__)) \
            { \
                Platform::TriggerDebugger(); \
            }                                                   

        #define SM_ASSERT_MSG(expr, msg) \
            if(expr){} \
            else if(Platform::AssertReportFailureMsg(#expr, msg, __FILE__, __LINE__)) \
            { \
                Platform::TriggerDebugger(); \
            }                                                   

        #define SM_ERROR() \
            if(Platform::AssertReportError(__FILE__, __LINE__)) \
            { \
                Platform::TriggerDebugger(); \
            }                                                   

        #define SM_ERROR_MSG(msg) \
            if(Platform::AssertReportErrorMsg(msg, __FILE__, __LINE__)) \
            { \
                Platform::TriggerDebugger(); \
            }                                                   
    #else
        #define SM_ASSERT
        #define SM_ASSERT_MSG
        #define SM_ERROR
        #define SM_ERROR_MSG
    #endif
}
