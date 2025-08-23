//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

typedef struct object_impl list_t;

list_t* list_alloc(allocator_t* allocator, size_t capacity);
size_t list_count(const list_t* list);
size_t list_capacity(const list_t* list);
void list_add(list_t* list, object_t* value);
object_t* list_pop(list_t* list);
object_t* list_get(const list_t* list, size_t index);
void list_clear(list_t* list);
bool list_empty(const list_t* list);
int list_find(const list_t* list, const object_t* value);
int list_find_predicate(const list_t* list, bool (*predicate) (const object_t*, void* data), void* data);

inline void list_push(list_t* list, object_t* value) { list_add(list, value); }
