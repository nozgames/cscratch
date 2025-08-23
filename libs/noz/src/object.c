//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

typedef struct object_impl
{
	// todo: add a debug magic number here to validate object integrity
	type_t type;
	type_t base_type;
} object_impl_t;

static inline object_impl_t* to_impl(const void* o) 
{
	return (object_impl_t*)o;
}

static void* default_alloc(allocator_t* a, size_t size)
{
	return malloc(size);
}

static void default_free(allocator_t* a, void* ptr)
{
	free(ptr);
}

static allocator_t default_allocator = {
	.alloc = default_alloc,
	.free = default_free
};

extern allocator_t* g_default_allocator = &default_allocator;

object_t* object_alloc_with_base(allocator_t* allocator, size_t object_size, type_t object_type, type_t base_type)
{
    object_impl_t* impl = to_impl(allocator_alloc(allocator, object_size));
    if (!impl)
		return NULL;

	impl->type = object_type;
	impl->base_type = base_type;
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
		text_t msg;
		text_init(&msg);
		text_format(&msg, "OBJECT_BASE_SIZE != %zu", object_type_size);
		application_error(msg.value);
		return;
	}
}

void object_uninit()
{
}
