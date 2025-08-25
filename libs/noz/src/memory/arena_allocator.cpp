//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

// todo: application trait
#define ARENA_ALLOCATOR_MAX_STACK 64

struct ArenaAllocator
{
    Allocator base;
    void* data;
    size_t* stack;
    size_t stack_depth;
    size_t stack_size;
    size_t stack_overflow;
    size_t size;
    size_t used;
};

void* ArenaAlloc(Allocator* a, size_t size)
{
    ArenaAllocator* impl = (ArenaAllocator*)a;

    // Align size to pointer boundary (8 bytes on 64-bit, 4 bytes on 32-bit)
    const size_t alignment = sizeof(void*);
    size_t aligned_size = (size + alignment - 1) & ~(alignment - 1);

    if (impl->used + aligned_size <= impl->size)
    {
        void* ptr = (char*)impl->data + impl->used;
        impl->used += aligned_size;
        return ptr;
    }

    // error: out of memory
    return nullptr;
}

void* ArenaRealloc(Allocator* a, void* ptr, size_t new_size)
{
    Exit("arena_allocator_realloc not supported");
    return nullptr;
}

void ArenaFree(Allocator* a, void* ptr)
{
    (void)a;
    (void)ptr;
}

// Reset the entire arena
void ArenaClear(Allocator* a)
{
    auto* impl = (ArenaAllocator*)a;
    assert(impl);
    impl->stack[0] = 0;
    impl->stack_depth = 0;
    impl->stack_overflow = 0;
    impl->used = 0;
}

void ArenaPush(Allocator* a)
{
    auto impl = (ArenaAllocator*)a;
    assert(impl);
    if (impl->stack_depth < impl->stack_size)
        impl->stack[impl->stack_depth++] = impl->used;
    else
        impl->stack_overflow++;
}

void ArenaPop(Allocator* a)
{
    auto* impl = (ArenaAllocator*)a;
    assert(impl);
    if (impl->stack_overflow > 0)
        impl->stack_overflow--;
    else if (impl->stack_depth > 0)
        impl->used = impl->stack[--impl->stack_depth];
    else
        // error: stack underflow
        ;
}

AllocatorStats ArenaStats(Allocator* a)
{
    auto* impl = (ArenaAllocator*)a;
    assert(impl);

    return { impl->size, impl->used };
}

Allocator* CreateArenaAllocator(size_t size, const char* name)
{
    auto* allocator = (ArenaAllocator*)calloc(
        1,
        sizeof(ArenaAllocator) +
        size +
        sizeof(size_t) * ARENA_ALLOCATOR_MAX_STACK);

    if (!allocator)
        return nullptr;

    allocator->base = {
        .alloc = ArenaAlloc,
        .free = ArenaFree,
        .realloc = ArenaRealloc,
        .push = ArenaPush,
        .pop = ArenaPop,
        .clear = ArenaClear,
        .stats = ArenaStats,
        .name = name,
    };
    allocator->stack = (size_t*)(allocator + 1);
    allocator->stack_size = ARENA_ALLOCATOR_MAX_STACK;
    allocator->size = size;
    allocator->data = (char*)(allocator->stack + ARENA_ALLOCATOR_MAX_STACK);
    return (Allocator*)allocator;
}

