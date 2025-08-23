//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

uint64_t hash_64(const void* data, size_t size);
uint64_t hash_string(const char* str);
uint64_t hash_name(const name_t* name);

#define hash_combine(...) hash_64_combine(__VA_ARGS__, 0)

// Implementation detail - don't call directly
uint64_t hash_64_combine_impl(const void* data, size_t size, uint64_t seed);

// Helper to combine multiple hash values
static inline uint64_t hash_64_combine(uint64_t h1, uint64_t h2, uint64_t h3) 
{
    uint64_t result = h1;
    if (h2) result = hash_64_combine_impl(&h2, sizeof(h2), result);
    if (h3) result = hash_64_combine_impl(&h3, sizeof(h3), result);
    return result;
}