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

static inline list_impl_t* Impl(void* l) { return (list_impl_t*)to_object((Object*)l, type_list); }

List* CreateList(Allocator* allocator, size_t capacity)
{   
    if (capacity == 0)
		capacity = DEFAULT_CAPACITY;
    
	list_impl_t* list = Impl(Alloc(allocator, sizeof(list_impl_t), type_list));
    if (!list)
        return NULL;
    
    list->count = 0;
    list->capacity = capacity;
	list->values = (void**)Alloc(allocator, sizeof(void*) * capacity);
    if (!list->values)
    {
        FreeObject((List*)list);
        return NULL;
    }
    
    return (List*)list;
}

// todo: destructor
#if 0
void list_free(List* list)
{
    if (!list) return;
    
    if (list->data)
        free(list->data);
    free(list);
}
#endif

size_t GetCount(List* list)
{
    return Impl(list)->count;
}

size_t GetCapacity(List* list)
{
    return Impl(list)->capacity;
}

void Add(List* list, void* value)
{
    list_impl_t* impl = Impl(list);
    
    if (impl->count >= impl->capacity)
    {
        impl->capacity *= 2;
		impl->values = (void**)Realloc(GetAllocator(list), impl->values, sizeof(void*) * impl->capacity);
        if (!impl->values)
            return;
    }
    
	impl->values[impl->count++] = value;
}

void* Pop(List* list)
{
    list_impl_t* impl = Impl(list);
    if (impl->count == 0)
        return NULL;
    
    return impl->values[--impl->count];
}

void* GetAt(List* list, size_t index)
{
    list_impl_t* impl = Impl(list);
	assert(index < impl->count);
	return impl->values[index];
}

void Clear(List* list)
{
    list_impl_t* impl = Impl(list);
    impl->count = 0;
}

bool IsEmpty(List* list)
{
    return Impl(list)->count == 0;
}

int Find(List* list, void* value)
{
	list_impl_t* impl = Impl(list);
    for (size_t i = 0; i < impl->count; i++)
        if (impl->values[i] == value)
            return (int)i;

    return -1;
}

int Find(List* list, bool (*predicate) (void*, void* data), void* data)
{
    assert(predicate);
    list_impl_t* impl = Impl(list);
    for (size_t i = 0; i < impl->count; i++)
        if (predicate(impl->values[i], data))
            return (int)i;

    return -1;
}
