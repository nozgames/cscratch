//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

typedef struct pool_entry
{
	struct pool_entry* next;
} pool_entry_t;

typedef struct pool_allocator
{
	allocator_t base;
	pool_entry_t* entries;
	pool_entry_t* free;
	size_t count;
} pool_allocator_t;


void* pool_allocator_alloc(pool_allocator_t* a)
{
	assert(a);
	if (!a->free)
		return NULL;

	pool_entry_t* entry = a->free;
	a->free = entry->next;
	a->count++;
	return (void*)(entry + 1);
}

void* pool_allocator_realloc(arena_allocator_t* a, void* ptr, size_t new_size)
{
	application_error("pool_allocator_realloc not supported");
	return NULL;
}

void pool_allocator_free(pool_allocator_t* a, void* ptr)
{
	assert(a);
	if (!ptr)
		return;
	pool_entry_t* entry = (pool_entry_t*)((char*)ptr - sizeof(pool_entry_t));
	entry->next = a->free;
	a->count--;
	a->free = entry;
}

size_t pool_allocator_count(pool_allocator_t* a)
{
	assert(a);
	return a->count;
}

pool_allocator_t* pool_allocator_create(size_t entry_size, size_t entry_count)
{
	size_t stride = entry_size + sizeof(pool_entry_t);
	pool_allocator_t* a = (pool_allocator_t*)calloc(1, sizeof(pool_allocator_t) + stride * entry_count);
	a->base = (allocator_t){
		.alloc = (void* (*)(allocator_t*, size_t))pool_allocator_alloc,
		.free = (void (*)(allocator_t*, void*))pool_allocator_free,
		.realloc = (void* (*)(allocator_t*, void*, size_t))pool_allocator_realloc
	};
	a->entries = (pool_entry_t*)(a + 1);

	// link all entries into a free list
	pool_entry_t* prev = a->free = a->entries;
	for (size_t i = 1; i < entry_count - 1; i++)
	{
		pool_entry_t* entry = (pool_entry_t*)((char*)a->entries + i * stride);
		prev->next = entry;
		prev = entry;
	}

	return a;
}

void pool_allocator_destroy(pool_allocator_t* a)
{
	assert(a);
	free(a);
}
