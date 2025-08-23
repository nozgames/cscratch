//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//
#pragma once

// @allocator
typedef struct allocator
{
	void* (*alloc)(struct allocator*, size_t size);
	void (*free)(struct allocator*, void* ptr);
} allocator_t;

// @allocator
extern allocator_t* g_default_allocator;

inline void* allocator_alloc(allocator_t* a, size_t size) { return (a == NULL ? g_default_allocator : a)->alloc(a, size); }
inline void allocator_free(allocator_t* a, void* ptr) { (a == NULL ? g_default_allocator : a)->free(a, ptr); }

// @arena
typedef struct arena_allocator arena_allocator_t;

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

