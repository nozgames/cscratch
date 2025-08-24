//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//
#pragma once

// @allocator
struct Allocator
{
    void* (*alloc)(Allocator*, size_t size);
    void (*free)(Allocator*, void* ptr);
    void* (*realloc)(Allocator*, void* ptr, size_t new_size);
};

// @allocator
extern Allocator* g_default_allocator;

inline void* Alloc(Allocator* a, size_t size)
{
    return (a == nullptr ? g_default_allocator : a)->alloc(a, size);
}

inline void Free(Allocator* a, void* ptr)
{
    (a == nullptr ? g_default_allocator : a)->free(a, ptr);
}

inline void* Realloc(Allocator* a, void* ptr, size_t new_size)
{
    return (a == nullptr ? g_default_allocator : a)->realloc(a, ptr, new_size);
}

// @arena
typedef Allocator arena_allocator_t;

arena_allocator_t* arena_allocator_create(size_t size);
void* arena_allocator_alloc(arena_allocator_t* a, size_t size);
void arena_allocator_destroy(arena_allocator_t* a);
void arena_allocator_free(arena_allocator_t* a);
void arena_allocator_push(arena_allocator_t* a);
void arena_allocator_pop(arena_allocator_t* a);

// @pool
typedef struct pool_allocator pool_allocator_t;

pool_allocator_t* pool_allocator_create(size_t entry_size, size_t entry_count);
void pool_allocator_destroy(pool_allocator_t* a);
void* pool_allocator_alloc(pool_allocator_t* a);
void pool_allocator_free(pool_allocator_t* a, void* ptr);
size_t pool_allocator_count(pool_allocator_t* a);

