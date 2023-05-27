#pragma once

#include <cassert>

bool report_assert_failure(const char* expression, const char* filename, int line_number);

#define ASSERTIONS_ENABLED !NDEBUG

#if ASSERTIONS_ENABLED

#define TRIGGER_DEBUG() __debugbreak() 
#define ASSERT(expr) \
	if(expr){} \
	else if(report_assert_failure(#expr, __FILE__, __LINE__)) \
	{ \
		TRIGGER_DEBUG(); \
	}                                                   

#define VULKAN_ASSERT(vk_result) ASSERT(VK_SUCCESS == vk_result)

// TODO: Create ASSERT_MSG, ERROR, ERROR_MSG

#else

#define ASSERT
#define VULKAN_ASSERT

#endif
