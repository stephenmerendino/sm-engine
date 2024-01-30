#pragma once

#include <cassert>

bool ReportAssertFailureMsg(const char* expression, const char* msg, const char* filename, int lineNumber);
bool ReportAssertFailure(const char* expression, const char* filename, int lineNumber);
bool ReportErrorMsg(const char* msg, const char* filename, int lineNumber);
bool ReportError(const char* filename, int lineNumber);

#define ASSERTIONS_ENABLED !NDEBUG

#if ASSERTIONS_ENABLED

#define TRIGGER_DEBUG() __debugbreak() 
#define SM_ASSERT(expr) \
	if(expr){} \
	else if(ReportAssertFailure(#expr, __FILE__, __LINE__)) \
	{ \
		TRIGGER_DEBUG(); \
	}                                                   

#define SM_ASSERT_MSG(expr, msg) \
	if(expr){} \
	else if(ReportAssertFailureMsg(#expr, msg, __FILE__, __LINE__)) \
	{ \
		TRIGGER_DEBUG(); \
	}                                                   

#define SM_ERROR() \
	if(ReportError(__FILE__, __LINE__)) \
	{ \
		TRIGGER_DEBUG(); \
	}                                                   

#define SM_ERROR_MSG(msg) \
	if(ReportErrorMsg(msg, __FILE__, __LINE__)) \
	{ \
		TRIGGER_DEBUG(); \
	}                                                   

#else

#define SM_ASSERT
#define SM_ASSERT_MSG
#define SM_ERROR
#define SM_ERROR_MSG
#define SM_VULKAN_ASSERT

#endif
