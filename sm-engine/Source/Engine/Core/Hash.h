#pragma once

#include <string>

typedef size_t HashedValue;

HashedValue Hash(const char* str);
HashedValue Hash(const std::string& str);
