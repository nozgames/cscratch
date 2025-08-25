//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

struct Map
{
    size_t capacity;
    size_t count;
    u64* keys;
    void* data;
    size_t data_stride;
};

Map CreateMap(u64* keys, size_t capacity, void* data, size_t data_stride, size_t initial_count=0);
bool HasKey(const Map& map, u64 key);
void* GetValue(const Map& map, const char* key);
void* GetValue(const Map& map, u64 key);
void* SetValue(Map& map, const char* key, void* value = nullptr);
void* SetValue(Map& map, u64 key, void* value = nullptr);

#if 0
typedef void (*MapEnumeratePredicate)(u64 key, void* value, void* user_data);
void Remove(const Map& map, const char* key);
void Remove(const Map& map, u64 key);
void Clear(const Map& map);
void Enumerate(const Map& map, MapEnumeratePredicate callback, void* user_data);
#endif