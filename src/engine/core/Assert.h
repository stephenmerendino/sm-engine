#pragma once

#include <cassert>

bool ReportAssertFailure(const char* expression, const char* fileName, int lineNumber);

#define ASSERTIONS_ENABLED !NDEBUG

#if ASSERTIONS_ENABLED

#define TriggerDebug() __debugbreak() 
#define ASSERT(expr) \
	if(expr){} \
	else if(ReportAssertFailure(#expr, __FILE__, __LINE__)) \
	{ \
		TriggerDebug(); \
	}                                                   

#define VULKAN_ASSERT(vkResult) ASSERT(VK_SUCCESS == vkResult)

#else

#define ASSERT
#define VULKAN_ASSERT

#endif
