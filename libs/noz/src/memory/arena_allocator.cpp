//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

// todo: application trait
#define ARENA_ALLOCATOR_MAX_STACK 64

typedef struct arena_allocator_impl
{
	allocator_t base;
	void* data;
	size_t* stack;
	size_t stack_depth;
	size_t stack_size;
	size_t stack_overflow;
	size_t size;
	size_t used;

} arena_allocator_impl_t;

void* arena_allocator_alloc(arena_allocator_t* a, size_t size)
{
	arena_allocator_impl_t* impl = (arena_allocator_impl_t*)a;

	if (impl->used + size <= impl->size)
	{
		void* ptr = (char*)impl->data + impl->used;
		impl->used += size;
		return ptr;
	}
	else
	{
		// error: out of memory
		return NULL;
	}
}

void* arena_allocator_realloc(arena_allocator_t* a, void* ptr, size_t new_size)
{
	Exit("arena_allocator_realloc not supported");
	return NULL;
}

void arena_allocator_free(arena_allocator_t* a)
{
	arena_allocator_impl_t* impl = (arena_allocator_impl_t*)a;
	assert(impl);
	impl->stack[0] = 0;
	impl->stack_depth = 0;
	impl->stack_overflow = 0;
	impl->used = 0;
}

void arena_allocator_push(arena_allocator_t* a)
{
	arena_allocator_impl_t* impl = (arena_allocator_impl_t*)a;
	assert(impl);
	if (impl->stack_depth < impl->stack_size)
		impl->stack[impl->stack_depth++] = impl->used;
	else
		impl->stack_overflow++;
}

void arena_allocator_pop(arena_allocator_t* a)
{
	arena_allocator_impl_t* impl = (arena_allocator_impl_t*)a;
	assert(impl);
	if (impl->stack_overflow > 0)
		impl->stack_overflow--;
	else if (impl->stack_depth > 0)
		impl->used = impl->stack[--impl->stack_depth];
	else
		// error: stack underflow
		;
}

arena_allocator_t* arena_allocator_create(size_t size)
{
	// arena_allocator_t + data + stack
	arena_allocator_impl_t* allocator = (arena_allocator_impl_t*)calloc(
		1,
		sizeof(arena_allocator_impl_t) +
		size +
		sizeof(size_t) * ARENA_ALLOCATOR_MAX_STACK);
	if (!allocator)
		return nullptr;

	allocator->base = {
		.alloc = (void* (*)(allocator_t*, size_t))arena_allocator_alloc,
		.free = (void (*)(allocator_t*, void*))arena_allocator_free,
		.realloc = (void* (*)(allocator_t*, void*, size_t))arena_allocator_realloc
	};
	allocator->stack = (size_t*)(allocator + 1);
	allocator->stack_size = ARENA_ALLOCATOR_MAX_STACK;
	allocator->size = size;
	allocator->data = (size_t*)(allocator->stack + ARENA_ALLOCATOR_MAX_STACK);
	return (arena_allocator_t*)allocator;
}

void arena_allocator_destroy(arena_allocator_t* a)
{
	free(a);
}
