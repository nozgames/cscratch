//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

typedef struct free_node
{
    struct free_node* next;
} free_node_t;

typedef struct object_pool_impl {
    void* memory;
    object_type_t object_type;
    size_t object_size;
    size_t capacity;
    size_t used_count;
    free_node_t* free_list;
} object_pool_impl;

static object_type_t g_object_pool_type = NULL;

void object_pool_init() 
{
    g_object_pool_type = object_type_create("object_pool");
}

object_pool_t object_pool_create(object_type_t object_type, size_t object_size, size_t capacity) {
    assert(object_type);
    assert(object_size >= sizeof(free_node_t));
    assert(capacity > 0);

    object_t obj = object_create(g_object_pool_type, sizeof(object_pool_impl));
    if (!obj) return NULL;

    object_pool_impl* pool = (object_pool_impl*)object_impl(obj, g_object_pool_type);

    pool->memory = malloc(object_size * capacity);
    if (!pool->memory) {
        object_destroy(obj);
        return NULL;
    }

    pool->object_type = object_type;
    pool->object_size = object_size;
    pool->capacity = capacity;
    pool->used_count = 0;
    pool->free_list = NULL;

    // Initialize free list
    for (size_t i = 0; i < capacity; i++) {
        void* obj_mem = (char*)pool->memory + (i * object_size);
        free_node_t* node = (free_node_t*)obj_mem;
        node->next = pool->free_list;
        pool->free_list = node;
    }

    return (object_pool_t)obj;
}

object_pool_t object_pool_create_with_base(object_type_t object_type, size_t object_size, object_type_t base_type, size_t base_size, size_t capacity) {
    assert(object_type);
    assert(base_type);
    assert(object_size >= base_size);
    assert(object_size >= sizeof(free_node_t));
    assert(capacity > 0);

    object_t obj = object_create(g_object_pool_type, sizeof(object_pool_impl));
    if (!obj) return NULL;

    object_pool_impl* pool = (object_pool_impl*)object_impl(obj, g_object_pool_type);

    pool->memory = malloc(object_size * capacity);
    if (!pool->memory) {
        object_destroy(obj);
        return NULL;
    }

    pool->object_type = object_type;
    pool->object_size = object_size;
    pool->capacity = capacity;
    pool->used_count = 0;
    pool->free_list = NULL;

    // Initialize free list
    for (size_t i = 0; i < capacity; i++) {
        void* obj_mem = (char*)pool->memory + (i * object_size);
        free_node_t* node = (free_node_t*)obj_mem;
        node->next = pool->free_list;
        pool->free_list = node;
    }

    return (object_pool_t)obj;
}

void object_pool_destroy(object_pool_t pool) {
    if (!pool) return;
    object_pool_impl* impl = (object_pool_impl*)object_impl((object_t)pool, g_object_pool_type);
    free(impl->memory);
    object_destroy((object_t)pool);
}

object_t object_pool_alloc(object_pool_t pool) {
    if (!pool) return NULL;
    object_pool_impl* impl = (object_pool_impl*)object_impl((object_t)pool, g_object_pool_type);
    
    if (!impl->free_list) return NULL;

    free_node_t* node = impl->free_list;
    impl->free_list = node->next;
    impl->used_count++;

    return (object_t)node;
}

void object_pool_free(object_pool_t pool, object_t object) {
    if (!pool || !object) return;
    object_pool_impl* impl = (object_pool_impl*)object_impl((object_t)pool, g_object_pool_type);

    free_node_t* node = (free_node_t*)object;
    node->next = impl->free_list;
    impl->free_list = node;
    impl->used_count--;
}

size_t object_pool_capacity(object_pool_t pool) {
    if (!pool) return 0;
    object_pool_impl* impl = (object_pool_impl*)object_impl((object_t)pool, g_object_pool_type);
    return impl->capacity;
}

size_t object_pool_used_count(object_pool_t pool) {
    if (!pool) return 0;
    object_pool_impl* impl = (object_pool_impl*)object_impl((object_t)pool, g_object_pool_type);
    return impl->used_count;
}

size_t object_pool_free_count(object_pool_t pool) {
    if (!pool) return 0;
    object_pool_impl* impl = (object_pool_impl*)object_impl((object_t)pool, g_object_pool_type);
    return impl->capacity - impl->used_count;
}