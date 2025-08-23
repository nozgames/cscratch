//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#define OBJECT_BASE_SIZE (sizeof(uint16_t) * 2)
#define OBJECT_BASE char __base[OBJECT_BASE_SIZE]

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

inline type_t object_type(const object_t* object) { return *(((type_t*)object) + 0); }
inline type_t object_base_type(const object_t* object) { return *(((type_t*)object) + 1); }
