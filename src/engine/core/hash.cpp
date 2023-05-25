#include "engine/core/hash.h"
#include <functional>

hash_id_t hash(const char* str)
{
    std::string temp_str(str); 
    return hash(temp_str);
}

hash_id_t hash(const std::string& str)
{
    return std::hash<std::string>{}(str); 
}
