//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

static void* default_alloc(Allocator* a, size_t size)
{
    return malloc(size);
}

static void* default_realloc(Allocator* a, void* ptr, size_t new_size)
{
    return realloc(ptr, new_size);
}

static void default_free(Allocator* a, void* ptr)
{
    free(ptr);
}

static Allocator default_allocator = {
    .alloc = default_alloc,
    .free = default_free,
    .realloc = default_realloc
};

Allocator* g_default_allocator = &default_allocator;
