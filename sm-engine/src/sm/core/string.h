#include "sm/core/array.h"

namespace sm
{
    struct string_t
    {
        array_t<char> c_str;

        char&       operator[](size_t index);
        const char& operator[](size_t index) const;
        void        operator=(const char* str);
        void        operator=(const string_t& str);
        string_t&   operator+=(const char* str);
        string_t&   operator+=(const string_t& str);
    };

    string_t string_init(sm::arena_t* arena, size_t initial_capacity = 64);
    size_t   string_calc_length(const string_t& str);
    wchar_t* string_to_wchar(sm::arena_t* arena, const string_t& s);
    wchar_t* string_to_wchar(sm::arena_t* arena, const char* s);
    void     string_to_wchar(wchar_t* wchar_string_memory, size_t max_num_chars, const string_t& s);
    void     string_to_wchar(wchar_t* wchar_string_memory, size_t max_num_chars, const char* s);
}
