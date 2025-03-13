#pragma once

#include "sm/memory/arena.h"
#include "sm/core/assert.h"

#include <cstring>

namespace sm
{
    // static_array
    template<typename T>
    struct array_t
    {
        T*     data = nullptr;
        size_t cur_size = 0;
        size_t max_capacity = 0;
        arena_t* arena = nullptr;

        T& operator[](size_t index)
        {
            SM_ASSERT(index < cur_size);
            return data[index];
        }

        const T& operator[](size_t index) const
        {
            SM_ASSERT(index < cur_size);
            return data[index];
        }
    };

    template<typename T>
    array_t<T> init_array(sm::arena_t* arena, size_t starting_capacity)
    {
        array_t<T> arr;
        arr.data = (T*)sm::alloc_array_zero(arena, T, starting_capacity);
        arr.cur_size = 0;
        arr.max_capacity = starting_capacity;
        arr.arena = arena;
        return arr;
    }

    template<typename T>
    array_t<T> init_array_sized(sm::arena_t* arena, size_t starting_capacity_and_size)
    {
        array_t<T> arr = init_array<T>(arena, starting_capacity_and_size);
        arr.cur_size = starting_capacity_and_size;
        return arr;
    }

    template<typename T>
    void copy(array_t<T>& dst_array, const T* src_data, size_t num_items)
    {
        SM_ASSERT(num_items <= dst_array.cur_size);
        memcpy(dst_array.data, src_data, num_items * sizeof(T));
    }

    template<typename T>
    void copy(array_t<T>& dst_array, array_t<T>& src_array, size_t num_items)
    {
        SM_ASSERT(num_items <= src_array.cur_size);
        SM_ASSERT(num_items <= dst_array.cur_size);
        memcpy(dst_array.data, src_array.data, num_items * sizeof(T));
    }

    template<typename T>
    void grow_capacity(array_t<T>& arr, size_t new_capacity)
    {
        if(new_capacity <= arr.max_capacity) return;

        T* old_data = arr.data;
        arr.data = (T*)sm::alloc_array_zero(arr.arena, T, new_capacity);
        arr.max_capacity = new_capacity;
        copy(arr, old_data, arr.cur_size);
    }

    template<typename T>
    void resize(array_t<T>& arr, size_t new_size)
    {
        grow_capacity(arr, new_size);
        arr.cur_size = new_size;
    }

    template<typename T>
    void push(array_t<T>& arr, T value)
    {
        if(arr.cur_size == arr.max_capacity)
        {
            grow_capacity(arr, arr.max_capacity * 2);
        }

        arr.data[arr.cur_size] = value;
        arr.cur_size++;
    }

    template<typename T>
    void push(array_t<T>& arr, const T* values, size_t num_values)
    {
        size_t needed_size = arr.cur_size + num_values;
        if(needed_size > arr.max_capacity)
        {
            grow_capacity(arr, needed_size);
        }

        memcpy(arr.data + (arr.cur_size * sizeof(T)), values, num_values * sizeof(T));
        arr.cur_size += num_values;
    }

    template<typename T>
    void push(array_t<T>& arr, const array_t<T>& new_values)
    {
        push(arr, new_values.data, new_values.cur_size);
    }
}
