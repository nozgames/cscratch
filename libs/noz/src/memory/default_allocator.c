//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

static void* default_alloc(allocator_t* a, size_t size)
{
	return malloc(size);
}

static void* default_realloc(allocator_t* a, void* ptr, size_t new_size)
{
	return realloc(ptr, new_size);
}

static void default_free(allocator_t* a, void* ptr)
{
	free(ptr);
}

static allocator_t default_allocator = {
	.alloc = default_alloc,
	.free = default_free,
	.realloc = default_realloc
};

extern allocator_t* g_default_allocator = &default_allocator;
