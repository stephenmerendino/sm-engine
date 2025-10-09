#include "sm/core/array.h"

namespace sm
{
    struct string_t
    {
        array_t<char> c_str;

        char&       operator[](size_t index);
        const char& operator[](size_t index) const;
    };

    string_t string_init(sm::arena_t* arena, size_t initial_capacity = 64);
    size_t   string_length(const string_t& str);
    void     string_set(string_t& string, const char* data);
    void     string_set(string_t& dst, const string_t& src);
    void     string_append(string_t& string, const char* data);
    void     string_append(string_t& dst, const string_t& src);

    wchar_t* string_to_wchar(sm::arena_t* arena, const string_t& s);
    wchar_t* string_to_wchar(sm::arena_t* arena, const char* s);
    void     string_to_wchar(wchar_t* wchar_string_memory, size_t max_num_chars, const string_t& s);
    void     string_to_wchar(wchar_t* wchar_string_memory, size_t max_num_chars, const char* s);

    #define CONCATENATE_DIRECT(s1, s2) s1##s2
    #define CONCATENATE(s1, s2) CONCATENATE_DIRECT(s1, s2)
}
