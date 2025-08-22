#pragma once

#include <stddef.h>

typedef struct object_type_impl* object_type_t;
typedef struct object_tag* object_t;
typedef struct object_pool* object_pool_t;
typedef struct object_registry* object_registry_t;

// @object_typ
object_type_t object_type_create(const char* name);
const char* object_type_name(object_type_t type);

// @object
object_t object_create(object_type_t object_type, size_t object_size);
object_t object_create_with_base(object_type_t object_type, size_t object_size, object_type_t base_type, size_t base_size);
void object_destroy(object_t object);
object_type_t object_type(object_t object);
object_type_t object_base_type(object_t object);
void* object_base_impl(object_t object, object_type_t expected_type);
void* object_impl(object_t object, object_type_t expected_type);

// @object_pool
object_pool_t object_pool_create(object_type_t object_type, size_t object_size, size_t capacity);
object_pool_t object_pool_create_with_base(object_type_t object_type, size_t object_size, object_type_t base_type, size_t base_size, size_t capacity);
void object_pool_destroy(object_pool_t pool);
object_t object_pool_alloc(object_pool_t pool);
void object_pool_free(object_pool_t pool, object_t object);
size_t object_pool_capacity(object_pool_t pool);
size_t object_pool_used_count(object_pool_t pool);
size_t object_pool_free_count(object_pool_t pool);

// @object_registry
object_registry_t object_registry_create(object_type_t object_type, size_t object_size, size_t capacity);
void object_registry_destroy(object_registry_t registry);
object_t object_registry_get(object_registry_t registry, const char* name);
object_t object_registry_alloc(object_registry_t registry, const char* name);
void object_registry_free(object_registry_t registry, const char* name);
size_t object_registry_count(object_registry_t registry);

#define nullptr NULL