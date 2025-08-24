//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

struct Map : Object {};

Map* AllocMap(Allocator* allocator, size_t capacity);
void* map_get_string(Map* map, char* key);
void* MapGet(Map* map, uint64_t key);
void map_set_string(Map* map, char* key, void* value);
void MapSet(Map* map, uint64_t key, void* value);
void map_remove_string(Map* map, char* key);
void map_remove(Map* map, uint64_t key);
void map_clear(Map* map);

typedef void (*map_iterate_fn)(uint64_t key, void* value, void* user_data);
void map_iterate(Map* map, map_iterate_fn callback, void* user_data);