#pragma once

#include "engine/core/assert.h"
#include "engine/core/types.h"
//#include <combaseapi.h>

//typedef GUID guid_t;
typedef u32 guid_t;

inline 
guid_t guid_generate()
{
    static int id_counter = 0;
    //GUID guid;
    //HRESULT res = ::CoCreateGuid(&guid);
    //ASSERT(SUCCEEDED(res));
    return id_counter++;
}
