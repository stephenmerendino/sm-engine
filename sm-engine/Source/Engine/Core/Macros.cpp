#include "Engine/Core/Macros.h"

ScopedFree::ScopedFree(void* ptr)
	:m_ptr(ptr)
{
}

ScopedFree::~ScopedFree()
{
	delete m_ptr;
}
