//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

typedef struct object_impl
{
	// todo: add a debug magic number here to validate object integrity
	type_t type;
	type_t base_type;
	uint32_t size;
	allocator_t* allocator;
} object_impl_t;

static inline object_impl_t* to_impl(const void* o) 
{
	return (object_impl_t*)o;
}

object_t* object_alloc_with_base(allocator_t* allocator, size_t object_size, type_t object_type, type_t base_type)
{
    object_impl_t* impl = to_impl(allocator_alloc(allocator, object_size));
    if (!impl)
		return NULL;

	impl->type = object_type;
	impl->base_type = base_type;
	impl->allocator = allocator;
	impl->size = (uint32_t)object_size;
    return impl;
}

void object_free(object_t* o)
{
	// todo: we would need to know the allocator to free this...  we could store it in the impl struct
}

void object_init()
{
	size_t object_type_size = sizeof(object_impl_t);
	if (object_type_size != OBJECT_BASE_SIZE)
	{
		application_error("OBJECT_BASE_SIZE != %zu", object_type_size);
		return;
	}

	size_t offset = offsetof(object_impl_t, type);
	if (offset != OBJECT_OFFSET_TYPE)
	{
		application_error("OBJECT_OFFSET_TYPE != %zu", object_type_size);
		return;
	}

	offset = offsetof(object_impl_t, base_type);
	if (offset != OBJECT_OFFSET_BASE)
	{
		application_error("OBJECT_OFFSET_BASE != %zu", offset);
		return;
	}

	offset = offsetof(object_impl_t, allocator);
	if (offset != OBJECT_OFFSET_ALLOCATOR)
	{
		application_error("OBJECT_OFFSET_ALLOCATOR != %zu", offset);
		return;
	}

	offset = offsetof(object_impl_t, size);
	if (offset != OBJECT_OFFSET_SIZE)
	{
		application_error("OBJECT_OFFSET_SIZE != %zu", offset);
		return;
	}
}

void object_uninit()
{
}
