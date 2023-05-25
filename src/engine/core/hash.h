#pragma once

#include <string>

typedef size_t hash_id_t;

hash_id_t hash(const char* str);
hash_id_t hash(const std::string& str);
