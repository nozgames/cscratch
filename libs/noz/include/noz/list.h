//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

typedef struct object_impl list_t;

list_t* list_alloc(allocator_t* allocator, size_t capacity);
size_t list_count(list_t* list);
size_t list_capacity(list_t* list);
void list_add(list_t* list, void* value);
void* list_pop(list_t* list);
void* list_get(list_t* list, size_t index);
void list_clear(list_t* list);
bool list_empty(list_t* list);
int list_find(list_t* list, void* value);
int list_find_predicate(list_t* list, bool (*predicate) (void*, void* data), void* data);

inline void list_push(list_t* list, void* value) { list_add(list, value); }
