#pragma once

namespace sm
{
    #define MAX_ASSERT_MSG_LEN 2048

    bool assert_report_failure_msg(const char* expression, const char* msg, const char* filename, int line_number);
    bool assert_report_failure(const char* expression, const char* filename, int line_number);
    bool assert_report_error_msg(const char* msg, const char* filename, int line_number);
    bool assert_report_error(const char* filename, int line_number);
    void trigger_debugger();

    #define ASSERTIONS_ENABLED !NDEBUG

    #if ASSERTIONS_ENABLED
        #define SM_ASSERT(expr) \
            if(expr){} \
            else if(assert_report_failure(#expr, __FILE__, __LINE__)) \
            { \
                trigger_debugger(); \
            }                                                   

        #define SM_ASSERT_MSG(expr, msg) \
            if(expr){} \
            else if(assert_report_failure_msg(#expr, msg, __FILE__, __LINE__)) \
            { \
                trigger_debugger(); \
            }                                                   

        #define SM_ERROR() \
            if(assert_report_error(__FILE__, __LINE__)) \
            { \
                trigger_debugger(); \
            }                                                   

        #define SM_ERROR_MSG(msg) \
            if(assert_report_error_msg(msg, __FILE__, __LINE__)) \
            { \
                trigger_debugger(); \
            }                                                   
    #else
        #define SM_ASSERT
        #define SM_ASSERT_MSG
        #define SM_ERROR
        #define SM_ERROR_MSG
    #endif
}
