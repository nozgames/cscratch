//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include <noz/list.h>

#define DEFAULT_CAPACITY 32

typedef struct list_impl
{
    OBJECT_BASE;
    void** values;
    size_t count;
    size_t capacity;
} list_impl_t;

static inline list_impl_t* to_impl(void* l) { return (list_impl_t*)to_object((object_t*)l, type_list); }

list_t* list_alloc(allocator_t* allocator, size_t capacity)
{   
    if (capacity == 0)
		capacity = DEFAULT_CAPACITY;
    
	list_impl_t* list = to_impl(object_alloc(allocator, sizeof(list_impl_t), type_list));
    if (!list)
        return NULL;
    
    list->count = 0;
    list->capacity = capacity;
	list->values = (void**)allocator_alloc(allocator, sizeof(void*) * capacity);
    if (!list->values)
    {
        object_free((list_t*)list);
        return NULL;
    }
    
    return (list_t*)list;
}

// todo: destructor
#if 0
void list_free(list_t* list)
{
    if (!list) return;
    
    if (list->data)
        free(list->data);
    free(list);
}
#endif

size_t list_count(list_t* list)
{
    return to_impl(list)->count;
}

size_t list_capacity(list_t* list)
{
    return to_impl(list)->capacity;
}

void list_add(list_t* list, void* value)
{
    list_impl_t* impl = to_impl(list);
    
    if (impl->count >= impl->capacity)
    {
        impl->capacity *= 2;
		impl->values = (void**)allocator_realloc(object_allocator(list), impl->values, sizeof(void*) * impl->capacity);
        if (!impl->values)
            return;
    }
    
	impl->values[impl->count++] = value;
}

void* list_pop(list_t* list)
{
    list_impl_t* impl = to_impl(list);
    if (impl->count == 0)
        return NULL;
    
    return impl->values[--impl->count];
}

void* list_get(list_t* list, size_t index)
{
    list_impl_t* impl = to_impl(list);
	assert(index < impl->count);
	return impl->values[index];
}

void list_clear(list_t* list)
{
    list_impl_t* impl = to_impl(list);
    impl->count = 0;
}

bool list_empty(list_t* list)
{
    return to_impl(list)->count == 0;
}

int list_find(list_t* list, void* value)
{
	list_impl_t* impl = to_impl(list);
    for (size_t i = 0; i < impl->count; i++)
        if (impl->values[i] == value)
            return (int)i;

    return -1;
}

int list_find_predicate(list_t* list, bool (*predicate) (void*, void* data), void* data)
{
    assert(predicate);
    list_impl_t* impl = to_impl(list);
    for (size_t i = 0; i < impl->count; i++)
        if (predicate(impl->values[i], data))
            return (int)i;

    return -1;
}
