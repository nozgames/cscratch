#include "object.h"
#include "string.h"
#include "uthash.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef struct registry_entry {
    string128 name;
    object_t object;
    UT_hash_handle hh;
} registry_entry_t;

typedef struct object_registry_impl {
    registry_entry_t* entries;
    object_pool_t pool;
    object_type_t object_type;
    size_t object_size;
} object_registry_impl;

static object_type_t registry_type = NULL;

object_registry_t object_registry_create(object_type_t object_type, size_t object_size, size_t capacity) {
    assert(object_type);
    assert(object_size > 0);
    assert(capacity > 0);

    if (!registry_type) {
        registry_type = object_type_register("object_registry");
    }

    object_t obj = object_create(registry_type, sizeof(object_registry_impl));
    if (!obj) return NULL;

    object_registry_impl* registry = (object_registry_impl*)object_impl(obj, registry_type);

    registry->pool = object_pool_create(object_size, capacity);
    if (!registry->pool) {
        object_destroy(obj);
        return NULL;
    }

    registry->entries = NULL;
    registry->object_type = object_type;
    registry->object_size = object_size;

    return (object_registry_t)obj;
}

void object_registry_destroy(object_registry_t registry) {
    if (!registry) return;
    object_registry_impl* impl = (object_registry_impl*)object_impl((object_t)registry, registry_type);

    registry_entry_t* current, *tmp;
    HASH_ITER(hh, impl->entries, current, tmp) {
        HASH_DEL(impl->entries, current);
        free(current);
    }

    object_pool_destroy(impl->pool);
    object_destroy((object_t)registry);
}

object_t object_registry_get(object_registry_t registry, const char* name) {
    if (!registry || !name) return NULL;
    object_registry_impl* impl = (object_registry_impl*)object_impl((object_t)registry, registry_type);

    registry_entry_t* entry;
    HASH_FIND_STR(impl->entries, name, entry);
    return entry ? entry->object : NULL;
}

object_t object_registry_alloc(object_registry_t registry, const char* name) {
    if (!registry || !name) return NULL;
    object_registry_impl* impl = (object_registry_impl*)object_impl((object_t)registry, registry_type);

    // Check if already exists
    registry_entry_t* entry;
    HASH_FIND_STR(impl->entries, name, entry);
    if (entry) return entry->object;

    // Allocate from pool
    object_t obj_data = object_pool_alloc(impl->pool);
    if (!obj_data) return NULL;

    // Create object
    object_t obj = object_create(impl->object_type, impl->object_size);
    if (!obj) {
        object_pool_free(impl->pool, obj_data);
        return NULL;
    }

    // Create registry entry
    entry = malloc(sizeof(registry_entry_t));
    if (!entry) {
        object_pool_free(impl->pool, obj_data);
        object_destroy(obj);
        return NULL;
    }

    string128_set(&entry->name, name);
    entry->object = obj;

    HASH_ADD_STR(impl->entries, name.data, entry);

    return obj;
}

void object_registry_free(object_registry_t registry, const char* name) {
    if (!registry || !name) return;
    object_registry_impl* impl = (object_registry_impl*)object_impl((object_t)registry, registry_type);

    registry_entry_t* entry;
    HASH_FIND_STR(impl->entries, name, entry);
    if (!entry) return;

    HASH_DEL(impl->entries, entry);
    
    object_destroy(entry->object);
    // Note: pool memory is reused via free list, no need to explicitly return to pool
    
    free(entry);
}

size_t object_registry_count(object_registry_t registry) {
    if (!registry) return 0;
    object_registry_impl* impl = (object_registry_impl*)object_impl((object_t)registry, registry_type);
    return HASH_COUNT(impl->entries);
}