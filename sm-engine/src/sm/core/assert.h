#pragma once

#include <cassert>

bool report_assert_failure_msg(const char* expression, const char* msg, const char* filename, int line_number);
bool report_assert_failure(const char* expression, const char* filename, int lineNumber);
bool report_error_msg(const char* msg, const char* filename, int lineNumber);
bool report_error(const char* filename, int lineNumber);

#define ASSERTIONS_ENABLED !NDEBUG

#if ASSERTIONS_ENABLED

#define TRIGGER_DEBUG() __debugbreak() 
#define SM_ASSERT(expr) \
	if(expr){} \
	else if(report_assert_failure(#expr, __FILE__, __LINE__)) \
	{ \
		TRIGGER_DEBUG(); \
	}                                                   

#define SM_ASSERT_MSG(expr, msg) \
	if(expr){} \
	else if(report_assert_failure_msg(#expr, msg, __FILE__, __LINE__)) \
	{ \
		TRIGGER_DEBUG(); \
	}                                                   

#define SM_ERROR() \
	if(report_error(__FILE__, __LINE__)) \
	{ \
		TRIGGER_DEBUG(); \
	}                                                   

#define SM_ERROR_MSG(msg) \
	if(report_error_msg(msg, __FILE__, __LINE__)) \
	{ \
		TRIGGER_DEBUG(); \
	}                                                   

#else

#define SM_ASSERT
#define SM_ASSERT_MSG
#define SM_ERROR
#define SM_ERROR_MSG

#endif
