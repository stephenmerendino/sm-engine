#include "SM/Util.h"
#include <cstring>
#include <cstdio>

using namespace SM;

char* SM::ConcatenateStrings(const char* s1, const char* s2, LinearAllocator* allocator)
{
    size_t combinedSize = strlen(s1) + strlen(s2) + 1;
    char* combinedString = (char*)allocator->Alloc(combinedSize);
    sprintf_s(combinedString, combinedSize, "%s%s\0", s1, s2);
    return combinedString;
}

const char* SM::ToString(bool b)
{
    return b ? "True" : "False";    
}
