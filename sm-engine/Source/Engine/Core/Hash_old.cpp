#include "Engine/Core/Hash_old.h"
#include <functional>

HashedValue Hash(const char* str)
{
	std::string temp_str(str);
	return Hash(temp_str);
}

HashedValue Hash(const std::string& str)
{
	return std::hash<std::string>{}(str);
}
