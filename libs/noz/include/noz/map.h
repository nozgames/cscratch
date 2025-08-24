//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

struct Map : Object {};

typedef void (*MapEnumeratePredicate)(u64 key, void* value, void* user_data);

Map* CreateMap(Allocator* allocator, size_t capacity);
void* GetValue(Map* map, const char* key);
void* GetValue(Map* map, u64 key);
void SetValue(Map* map, const char* key, void* value);
void SetValue(Map* map, u64 key, void* value);
void Remove(Map* map, const char* key);
void Remove(Map* map, u64 key);
void Clear(Map* map);
void Enumerate(Map* map, MapEnumeratePredicate callback, void* user_data);