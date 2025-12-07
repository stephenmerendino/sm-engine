#pragma once

#include "SM/StandardTypes.h"

namespace SM
{
    template<typename T, size_t MAX_SIZE>
    class Stack
    {
    public:
        void Push(const T& t);
        bool Pop();
        bool Top(T* t);

        U32 m_numItems = 0;
        T m_data[MAX_SIZE];
    };

    template<typename T, size_t MAX_SIZE>
    void Stack<T, MAX_SIZE>::Push(const T& t)
    {
        SM_ASSERT(m_numItems >= 0 && m_numItems < MAX_SIZE);
        m_data[m_numItems] = t;
        m_numItems++;
    }

    template<typename T, size_t MAX_SIZE>
    bool Stack<T, MAX_SIZE>::Pop()
    {
        if(m_numItems == 0)
            return false;

        m_numItems--;
        return true;
    }

    template<typename T, size_t MAX_SIZE>
    bool Stack<T, MAX_SIZE>::Top(T* t)
    {
        if(m_numItems == 0)
            return false;

        *t = m_data[m_numItems - 1];
        return true;
    }
}
