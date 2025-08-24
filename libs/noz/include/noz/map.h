//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

typedef struct object_impl map_t;

map_t* map_alloc(allocator_t* allocator, size_t capacity);
void* map_get_string(map_t* map, char* key);
void* map_get(map_t* map, uint64_t key);
void map_set_string(map_t* map, char* key, void* value);
void map_set(map_t* map, uint64_t key, void* value);
void map_remove_string(map_t* map, char* key);
void map_remove(map_t* map, uint64_t key);
void map_clear(map_t* map);

typedef void (*map_iterate_fn)(uint64_t key, void* value, void* user_data);
void map_iterate(map_t* map, map_iterate_fn callback, void* user_data);