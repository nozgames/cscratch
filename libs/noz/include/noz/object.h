//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#include "allocator.h"

#define OBJECT_BASE_SIZE (sizeof(u16) * 2 + sizeof(void*) + sizeof(u32))
#define OBJECT_BASE char __object[OBJECT_BASE_SIZE]
#define OBJECT_OFFSET_TYPE (0)
#define OBJECT_OFFSET_BASE (sizeof(u16))
#define OBJECT_OFFSET_SIZE (sizeof(u16) * 2)
#define OBJECT_OFFSET_ALLOCATOR (sizeof(u16) * 2 + sizeof(u32))


typedef int16_t type_t;
typedef struct object_impl object_t;

#define type_invalid (-1)

// @object
object_t* object_alloc_with_base(allocator_t* allocator, size_t object_size, type_t object_type, type_t base_type);
inline object_t* object_alloc(allocator_t* allocator, size_t object_size, type_t object_type)
{
	return object_alloc_with_base(allocator, object_size, object_type, -1);
}

void object_free(object_t* object);

inline type_t object_type(object_t* object) { return *((type_t*)((char*)object + OBJECT_OFFSET_TYPE)); }
inline type_t object_base_type(object_t* object) { return *((type_t*)((char*)object + OBJECT_OFFSET_BASE)); }
inline size_t object_size(object_t* object) { return (size_t)(uint32_t*)((char*)object + OBJECT_OFFSET_SIZE); }
inline allocator_t* object_allocator(object_t* object) { return *(allocator_t**)((char*)object + OBJECT_OFFSET_ALLOCATOR); }
