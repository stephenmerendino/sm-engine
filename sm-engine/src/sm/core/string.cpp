#include "sm/core/string.h"
#include <cstdlib>

using namespace sm;

char& static_string_t::operator[](size_t index)
{
    SM_ASSERT(index <= c_str.size);
    return c_str[index];
}

const char& static_string_t::operator[](size_t index) const
{
    SM_ASSERT(index <= c_str.size);
    return c_str[index];
}

void static_string_t::operator=(const char* str)
{
    size_t len = strlen(str);
    sm::copy(c_str, str, len + 1);
    c_str[len] = '\0';
}

void static_string_t::operator=(static_string_t str)
{
    *this = str.c_str.data;
}

static_string_t sm::init_static_string(sm::arena_t* arena, size_t size)
{
    static_string_t str;
    str.c_str = init_static_array<char>(arena, size);
    return str;
}

wchar_t* sm::to_wchar_string(sm::arena_t* arena, const static_string_t& s)
{
	return to_wchar_string(arena, s.c_str.data);
}

wchar_t* sm::to_wchar_string(sm::arena_t* arena, const char* s)
{
	size_t len = strlen(s);
	size_t num_chars_converted = 0;
	wchar_t* unicode_str = (wchar_t*)sm::alloc_array_zero(arena, wchar_t, len + 1);
	::mbstowcs_s(&num_chars_converted, unicode_str, len + 1, s, len);
	return unicode_str;
}