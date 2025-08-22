//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

// todo: application trait
#define ARENA_ALLOCATOR_MAX_STACK 64

typedef struct arena_allocator
{
	allocator_t base;
	void* data;
	size_t* stack;
	size_t stack_depth;
	size_t stack_size;
	size_t stack_overflow;
	size_t size;
	size_t used;

} arena_allocator_t;

void* arena_allocator_alloc(arena_allocator_t* a, size_t size)
{
	if (a->used + size <= a->size)
	{
		void* ptr = (char*)a->data + a->used;
		a->used += size;
		return ptr;
	}
	else
	{
		// error: out of memory
		return NULL;
	}
}

void arena_allocator_free(arena_allocator_t* a)
{
	assert(a);
	a->stack[0] = 0;
	a->stack_depth = 0;
	a->stack_overflow = 0;
	a->used = 0;
}

void arena_allocator_push(arena_allocator_t* a)
{
	if (a->stack_depth < a->stack_size)
		a->stack[a->stack_depth++] = a->used;
	else
		a->stack_overflow++;
}

void arena_allocator_pop(arena_allocator_t* a)
{
	if (a->stack_overflow > 0)
		a->stack_overflow--;
	else if (a->stack_depth > 0)
		a->used = a->stack[--a->stack_depth];
	else
		// error: stack underflow
		;
}

const arena_allocator_t* arena_allocator_create(size_t size)
{
	// arena_allocator_t + data + stack
	arena_allocator_t* allocator = (arena_allocator_t*)calloc(
		1,
		sizeof(arena_allocator_t) +
		size +
		sizeof(size_t) * ARENA_ALLOCATOR_MAX_STACK);
	allocator->base = (allocator_t) {
		.alloc = (void* (*)(allocator_t*, size_t))arena_allocator_alloc,
		.free = (void (*)(allocator_t*, void*))arena_allocator_free
	};
	allocator->stack = (size_t*)(allocator + 1);
	allocator->stack_size = ARENA_ALLOCATOR_MAX_STACK;
	allocator->size = size;
	allocator->data = (size_t*)(allocator->stack + ARENA_ALLOCATOR_MAX_STACK);
	return allocator;
}

void arena_allocator_destroy(arena_allocator_t* a)
{
	free(a);
}
