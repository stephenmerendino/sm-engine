#include "sm/core/hash.h"

#include <functional>
#include <string>

using namespace sm;

hash_t sm::hash(const static_string_t& str)
{
	return hash(str.c_str.data);
}

hash_t sm::hash(const char* str)
{
	return std::hash<std::string>{}(std::string(str));
}