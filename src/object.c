#include "object.h"
#include "string.h"

#define MAX_TYPES 256

struct object_type
{
    string128 name;
};

typedef struct object
{
    object_type_t type;
    object_type_t base_type;
    void* base_impl;
    void* impl;
} object;

static struct object_type g_types[MAX_TYPES] = {0};
static int g_type_count = 0;

object_type_t object_type_register(const char* name)
{
    assert(g_type_count < MAX_TYPES);

    struct object_type* type = g_types + g_type_count++;
    string128_set(&type->name, name);
    return type;
}

const char* object_type_name(object_type_t type)
{
    assert(type);
    return type->name.data;
}

object_t object_create(object_type_t type, size_t object_size)
{ 
    object* o = (object*)malloc(sizeof(object) + object_size);
    if (!o)
        return nullptr;

    o->type = type;
    o->base_impl = nullptr;
    o->base_type = nullptr;
    o->impl = (void*)(o + 1);
    return (object_t)o;
}

object_t object_create_with_base(object_type_t object_type, size_t object_size, object_type_t base_type, size_t base_size)
{
    object* o = (object*)malloc(sizeof(object) + base_size + object_size);
    if (!o)
        return nullptr;
    memset(o, 0, sizeof(object) + base_size + object_size);
    o->base_impl = (void*)(o + 1);
    o->base_type = base_type;
    o->impl = (void*)((char*)o->base_impl + base_size);
    o->type = object_type;
    return (object_t)o;
}

object_type_t object_type(object_t o)
{
    if (o == nullptr)
        return nullptr;
    object* impl = (object*)o;
    return impl->type;
}

object_type_t object_base_type(object_t o)
{
    if (o == nullptr)
        return nullptr;
    object* impl = (object*)o;
    return impl->base_type;
}

void* object_base_impl(object_t o, object_type_t expected_type)
{
    assert(o);
    object* impl = (object*)o;
    return impl->base_impl;
}

void object_destroy(object_t o)
{
    if (!o) return;
    free(o);
}

void* object_impl(object_t o, object_type_t expected_type)
{
    assert(o);
    object* impl = (object*)o;
    return impl->impl;
}
