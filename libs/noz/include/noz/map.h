//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

typedef struct map* map_t;

map_t map_create(void);
void* map_get_string(map_t map, const char* key);
void* map_get(map_t map, uint64_t key);
void map_set_string(map_t map, const char* key, void* value);
void map_set(map_t map, uint64_t key, void* value);
void map_remove_string(map_t map, const char* key);
void map_remove(map_t map, uint64_t key);