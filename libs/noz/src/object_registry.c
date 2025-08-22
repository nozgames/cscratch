//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

typedef struct registry_entry 
{
    uint64_t key;
    object_t object;
    UT_hash_handle hh;
} registry_entry_t;

typedef struct object_registry_impl 
{
    registry_entry_t* entries;
} object_registry_impl_t;

static object_type_t g_object_registry_type = NULL;

inline object_registry_impl_t* to_impl(object_registry_t registry)
{
    assert(registry);
    return (object_registry_impl_t*)object_impl((object_t)registry, g_object_registry_type);
}

object_registry_t object_registry_create(size_t capacity) 
{
    assert(object_type);
    assert(capacity > 0);

    object_registry_t registry = (object_registry_t)object_create(
        g_object_registry_type,
        sizeof(object_registry_impl_t) + sizeof(registry_entry_t) * capacity);
    if (!registry)
        return nullptr;

	object_registry_impl_t* impl = to_impl(registry);
    impl->entries = (registry_entry_t*)(impl + 1);
    return registry;
}

void object_registry_destroy(object_registry_t registry) 
{
	object_registry_t* impl = to_impl(registry);

    registry_entry_t* current;
    registry_entry_t* tmp;
    
    HASH_ITER(hh, impl->entries, current, tmp) 
    {
        HASH_DEL(impl->entries, current);
        free(current);
    }

    object_pool_destroy(impl->pool);
    object_destroy((object_t)registry);
}

object_t object_registry_get(object_registry_t registry, uint64_t key) 
{
    object_registry_impl_t* impl = to_impl(registry);

    registry_entry_t* entry;
    HASH_FIND(hh, impl->entries, &key, sizeof(uint64_t), entry);
    return entry ? entry->object : NULL;
}

void object_registry_set(object_registry_t registry, uint64_t key, object_t value) 
{
    object_registry_impl_t* impl = to_impl(registry);

    // exist already? if so replace
    registry_entry_t* entry;
    HASH_FIND(hh, impl->entries, &key, sizeof(uint64_t), entry);
    if (entry)
    {
		entry->object = value;
        return;
    }

    // todo: get free entry from list

    entry->key = key;
    entry->object = value;

    HASH_ADD(hh, impl->entries, key, sizeof(uint64_t), entry);
}

void object_registry_remove(object_registry_t registry, uint64_t key) 
{
    object_registry_impl_t* impl = to_impl(registry);

    registry_entry_t* entry;
    HASH_FIND(hh, impl->entries, &key, sizeof(uint64_t), entry);
    if (!entry) return;

    HASH_DEL(impl->entries, entry);
    
    object_destroy(entry->object);
    // Note: pool memory is reused via free list, no need to explicitly return to pool
    
    free(entry);
}

size_t object_registry_count(object_registry_t registry) 
{
    return to_impl(registry)->entries;
}

// Helper functions for string-based keys
object_t object_registry_get_string(object_registry_t registry, const char* name) 
{
    assert(name);
    return object_registry_get(registry, hash_string(name));
}

object_t object_registry_set_string(object_registry_t registry, const char* name, object_t value) 
{
    assert(name);
    object_registry_set(registry, hash_string(name), value);
}

void object_registry_remove_string(object_registry_t registry, const char* name) 
{
    object_registry_remove(registry, hash_string(name));
}

void object_registry_init()
{
    g_object_registry_type = object_type_create("object_registry");
}

void object_registry_uninit()
{
    g_object_registry_type = nullptr;
}   
