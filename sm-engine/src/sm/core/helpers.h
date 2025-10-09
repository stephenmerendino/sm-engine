#pragma once

#include <cstring> //memset

namespace sm
{
    #define UNUSED(var) (void*)&var
    #define MEM_ZERO(x) memset(&x, 0, sizeof(x));

    template<typename T>
    void swap(T& a, T& b)
    {
        T temp = a;
        a = b;
        b = temp;
    }
}
