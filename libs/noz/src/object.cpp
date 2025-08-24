//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

typedef struct ObjectImpl
{
    // todo: add a debug magic number here to validate object integrity
    type_t type;
    type_t base_type;
    uint32_t size;
    Allocator* allocator;
} object_impl_t;

static inline object_impl_t* Impl(const void* o) 
{
    return (object_impl_t*)o;
}

Object* Alloc(Allocator* allocator, size_t object_size, type_t object_type, type_t base_type)
{
    object_impl_t* impl = Impl(allocator_alloc(allocator, object_size));
    if (!impl)
        return nullptr;

    impl->type = object_type;
    impl->base_type = base_type;
    impl->allocator = allocator;
    impl->size = (uint32_t)object_size;
    return (Object*)impl;
}

void Free(Object* o)
{
    // todo: we would need to know the allocator to free this...  we could store it in the impl struct
}

void InitObject()
{
    size_t object_type_size = sizeof(object_impl_t);
    if (object_type_size != OBJECT_BASE_SIZE)
    {
        Exit("OBJECT_BASE_SIZE != %zu", object_type_size);
        return;
    }

    size_t offset = offsetof(object_impl_t, type);
    if (offset != OBJECT_OFFSET_TYPE)
    {
        Exit("OBJECT_OFFSET_TYPE != %zu", object_type_size);
        return;
    }

    offset = offsetof(object_impl_t, base_type);
    if (offset != OBJECT_OFFSET_BASE)
    {
        Exit("OBJECT_OFFSET_BASE != %zu", offset);
        return;
    }

    offset = offsetof(object_impl_t, allocator);
    if (offset != OBJECT_OFFSET_ALLOCATOR)
    {
        Exit("OBJECT_OFFSET_ALLOCATOR != %zu", offset);
        return;
    }

    offset = offsetof(object_impl_t, size);
    if (offset != OBJECT_OFFSET_SIZE)
    {
        Exit("OBJECT_OFFSET_SIZE != %zu", offset);
        return;
    }
}

void ShutdownObject()
{
}
