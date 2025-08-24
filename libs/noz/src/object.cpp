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

static_assert(OBJECT_BASE_SIZE == sizeof(ObjectImpl));
static_assert(OBJECT_OFFSET_TYPE == offsetof(ObjectImpl, type));
static_assert(OBJECT_OFFSET_BASE == offsetof(ObjectImpl, base_type));
static_assert(OBJECT_OFFSET_SIZE == offsetof(ObjectImpl, size));
static_assert(OBJECT_OFFSET_ALLOCATOR == offsetof(ObjectImpl, allocator));

static object_impl_t* Impl(const void* o)
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
