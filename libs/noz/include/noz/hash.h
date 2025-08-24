//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

uint64_t Hash(void* data, size_t size);
uint64_t Hash(const char* str);
uint64_t Hash(const name_t* name);

#define hash_combine(...) Hash(__VA_ARGS__, 0)

uint64_t Hash(void* data, size_t size, uint64_t seed);

static uint64_t Hash(uint64_t h1, uint64_t h2, uint64_t h3)
{
    uint64_t result = h1;
    if (h2) result = Hash(&h2, sizeof(h2), result);
    if (h3) result = Hash(&h3, sizeof(h3), result);
    return result;
}