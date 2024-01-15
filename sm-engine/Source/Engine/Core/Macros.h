#pragma once

#include <cstring> //memset

#define UNUSED(var) (void*)&var
#define MEM_ZERO(x) memset(&x, 0, sizeof(x));

#define CONCATENATE_DIRECT(s1, s2) s1##s2
#define CONCATENATE(s1, s2) CONCATENATE_DIRECT(s1, s2)

class ScopedFree
{
public:
	ScopedFree(void* ptr);
	~ScopedFree();
	void* m_ptr;
};
#define FREE_AFTER_SCOPE(ptr) ScopedFree CONCATENATE(scopedFree, __LINE__)((void*)ptr)
