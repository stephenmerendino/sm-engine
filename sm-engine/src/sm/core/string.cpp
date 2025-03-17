#include "sm/core/string.h"
#include <cstdlib>

using namespace sm;

char& string_t::operator[](size_t index)
{
    return c_str[index];
}

const char& string_t::operator[](size_t index) const
{
    return c_str[index];
}

void string_t::operator=(const char* str)
{
	c_str.cur_size = 0;
    size_t len = strlen(str);
	array_grow_capacity(c_str, len + 1);
	array_push(c_str, str, len);
	c_str.data[len] = '\0';
}

void string_t::operator=(const string_t& str)
{
    *this = str.c_str.data;
}

string_t& string_t::operator+=(const char* str)
{
	size_t additional_len = strlen(str);
	array_push(c_str, str, additional_len);
	c_str.data[c_str.cur_size] = '\0';
	return *this;
}

string_t& string_t::operator+=(const string_t& str)
{
	*this += str.c_str.data;
	return *this;
}

string_t sm::string_init(sm::arena_t* arena, size_t size)
{
    string_t str;
    str.c_str = array_init<char>(arena, size);
    return str;
}

size_t sm::string_calc_length(const string_t& str)
{
	return strlen(str.c_str.data);
}

wchar_t* sm::string_to_wchar(sm::arena_t* arena, const string_t& s)
{
	return string_to_wchar(arena, s.c_str.data);
}

wchar_t* sm::string_to_wchar(sm::arena_t* arena, const char* s)
{
	size_t len = strlen(s);
	size_t num_chars_converted = 0;
	wchar_t* unicode_str = (wchar_t*)sm::arena_alloc_array_zero(arena, wchar_t, len + 1);
	::mbstowcs_s(&num_chars_converted, unicode_str, len + 1, s, len);
	return unicode_str;
}

void sm::string_to_wchar(wchar_t* wchar_string_memory, size_t max_num_chars, const string_t& s)
{
	string_to_wchar(wchar_string_memory, max_num_chars, s.c_str.data);
}

void sm::string_to_wchar(wchar_t* wchar_string_memory, size_t max_num_chars, const char* s)
{
	size_t len = strlen(s);
	SM_ASSERT(len <= max_num_chars);
	size_t num_chars_converted = 0;
	::mbstowcs_s(&num_chars_converted, wchar_string_memory, len + 1, s, len);
}