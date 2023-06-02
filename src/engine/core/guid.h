#pragma once

#include "engine/core/assert.h"
#include "engine/core/types.h"
#include "engine/platform/windows_include.h"

typedef GUID guid_t;

inline 
guid_t guid_generate()
{
    GUID guid;
    HRESULT res = ::CoCreateGuid(&guid);
    SM_ASSERT(SUCCEEDED(res));
    return guid;
}
