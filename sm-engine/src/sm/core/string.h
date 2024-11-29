#include "sm/core/array.h"

namespace sm
{
    struct static_string_t
    {
        static_array_t<char> c_str;

        char&       operator[](size_t index);
        const char& operator[](size_t index) const;
        void        operator=(const char* str);
        void        operator=(static_string_t str);
    };

    static_string_t init_static_string(sm::arena_t* arena, size_t size);
    wchar_t* to_wchar_string(sm::arena_t* arena, const static_string_t& s);
    wchar_t* to_wchar_string(sm::arena_t* arena, const char* s);
    void     to_wchar_string(wchar_t* wchar_string_memory, size_t max_num_chars, const static_string_t& s);
    void     to_wchar_string(wchar_t* wchar_string_memory, size_t max_num_chars, const char* s);
}
