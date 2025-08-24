//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include <string.h>
#define XXH_STATIC_LINKING_ONLY
#define XXH_IMPLEMENTATION
#include <xxhash.h>

uint64_t hash_64(void* data, size_t size) 
{
    return XXH64(data, size, 0);
}

uint64_t hash_string(const char* str) 
{
    if (!str) return 0;
    return XXH64(str, strlen(str), 0);
}

uint64_t hash_name(name_t* name)
{
    if (name_empty(name)) return 0;
    return XXH64(name->value, name->length, 0);
}

uint64_t hash_64_combine_impl(void* data, size_t size, uint64_t seed) 
{
    return XXH64(data, size, seed);
}