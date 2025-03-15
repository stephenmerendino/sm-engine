#pragma once

#include <cassert>

bool ReportAssertFailureMsg(const char* expression, const char* msg, const char* filename, int lineNumber);
bool ReportAssertFailure(const char* expression, const char* filename, int lineNumber);
bool ReportErrorMsg(const char* msg, const char* filename, int lineNumber);
bool ReportError(const char* filename, int lineNumber);

#define ASSERTIONS_ENABLED !NDEBUG

#if ASSERTIONS_ENABLED

#define TRIGGER_DEBUG() __debugbreak() 
#define SM_ASSERT_OLD(expr) \
	if(expr){} \
	else if(ReportAssertFailure(#expr, __FILE__, __LINE__)) \
	{ \
		TRIGGER_DEBUG(); \
	}                                                   

#define SM_ASSERT_MSG_OLD(expr, msg) \
	if(expr){} \
	else if(ReportAssertFailureMsg(#expr, msg, __FILE__, __LINE__)) \
	{ \
		TRIGGER_DEBUG(); \
	}                                                   

#define SM_ERROR_OLD() \
	if(ReportError(__FILE__, __LINE__)) \
	{ \
		TRIGGER_DEBUG(); \
	}                                                   

#define SM_ERROR_MSG_OLD(msg) \
	if(ReportErrorMsg(msg, __FILE__, __LINE__)) \
	{ \
		TRIGGER_DEBUG(); \
	}                                                   

#else

#define SM_ASSERT_OLD
#define SM_ASSERT_MSG_OLD
#define SM_ERROR_OLD
#define SM_ERROR_MSG_OLD

#endif
